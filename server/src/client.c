#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

// 定义常量
#define SERVER_IP "192.168.7.95" 
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_ACTION_LEN 20
#define MAX_ONLINE_USERS 50

// 消息类型
typedef enum
{
    MSG_TYPE_AUTH = 1,    // 认证消息
    MSG_TYPE_PRIVATE,     // 私聊消息
    MSG_TYPE_GROUP,       // 群聊消息
    MSG_TYPE_ONLINE_LIST, // 请求在线列表
    MSG_TYPE_SYSTEM,      // 系统消息
    MSG_TYPE_EXIT         // 退出消息
} msg_type_t;

// 消息结构
typedef struct message_info
{
    msg_type_t type;                 // 消息类型
    char from[MAX_USERNAME_LEN];     // 发送者
    char to[MAX_USERNAME_LEN];       // 接收者
    char username[MAX_USERNAME_LEN]; // 用户名
    char password[MAX_PASSWORD_LEN]; // 密码
    char action[MAX_ACTION_LEN];     // 动作
    char content[BUFFER_SIZE];       // 消息内容
} message_t;

int sockfd;
char g_username[MAX_USERNAME_LEN] = {0}; // 全局用户名
bool is_authenticated = false;

// 显示帮助信息
void show_help()
{
    printf("\n=== 聊天室命令 ===\n");
    printf("/register <用户名> <密码>  - 注册新账号\n");
    printf("/login <用户名> <密码>     - 登录账号\n");
    printf("/logout                   - 退出登录\n");
    printf("/private <用户名> <消息>   - 发送私聊消息\n");
    printf("/group <消息>              - 发送群聊消息\n");
    printf("/list                      - 查看在线用户\n");
    printf("/help                      - 显示帮助\n");
    printf("/exit                      - 退出程序\n");
    printf("==========================\n\n");
}

// 发送消息到服务器
void send_message(message_t *msg)
{
    if (write(sockfd, msg, sizeof(message_t)) == -1)
    {
        perror("发送消息失败");
        exit(EXIT_FAILURE);
    }
}

// 注册用户
void register_user(const char *username, const char *password)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_AUTH;
    strcpy(msg.action, "register");
    strncpy(msg.username, username, MAX_USERNAME_LEN);
    strncpy(msg.password, password, MAX_PASSWORD_LEN);

    send_message(&msg);
}

// 登录用户
void login_user(const char *username, const char *password)
{
    if (is_authenticated)
    {
        printf("您已经登录，请先退出当前登录\n");
        return;
    }

    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_AUTH;
    strcpy(msg.action, "login");
    strncpy(msg.username, username, MAX_USERNAME_LEN);
    strncpy(msg.password, password, MAX_PASSWORD_LEN);

    send_message(&msg);

    // 注意：这里不立即设置g_username和is_authenticated
    // 等待服务器确认后再设置
    printf("登录请求已发送，等待服务器响应...\n");
}

// 退出登录
void logout_user()
{
    if (!is_authenticated)
    {
        printf("您尚未登录\n");
        return;
    }

    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_AUTH;
    strcpy(msg.action, "logout");
    strncpy(msg.username, g_username, MAX_USERNAME_LEN);

    send_message(&msg);
}

// 发送私聊消息
void send_private_message(const char *to, const char *content)
{
    if (!is_authenticated)
    {
        printf("请先登录\n");
        return;
    }

    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_PRIVATE;
    strncpy(msg.from, g_username, MAX_USERNAME_LEN);
    strncpy(msg.to, to, MAX_USERNAME_LEN);
    strncpy(msg.content, content, BUFFER_SIZE);

    send_message(&msg);
}

// 发送群聊消息
void send_group_message(const char *content)
{
    if (!is_authenticated)
    {
        printf("请先登录\n");
        return;
    }

    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_GROUP;
    strncpy(msg.from, g_username, MAX_USERNAME_LEN);
    strncpy(msg.content, content, BUFFER_SIZE);

    send_message(&msg);
}

// 请求在线用户列表
void request_online_list()
{
    if (!is_authenticated)
    {
        printf("请先登录\n");
        return;
    }

    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_ONLINE_LIST;
    strncpy(msg.from, g_username, MAX_USERNAME_LEN);

    send_message(&msg);
}

// 处理用户输入
void process_input()
{
    char input[BUFFER_SIZE];

    while (1)
    {
        if (is_authenticated)
        {
            printf("[%s] > ", g_username);
        }
        else
        {
            printf("[未登录] > ");
        }
        fflush(stdout);

        if (fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            break;
        }

        // 去除换行符
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0)
        {
            continue;
        }

        // 处理命令
        if (input[0] == '/')
        {
            char *command = strtok(input + 1, " ");

            if (strcmp(command, "register") == 0)
            {
                char *username = strtok(NULL, " ");
                char *password = strtok(NULL, " ");

                if (username && password)
                {
                    register_user(username, password);
                }
                else
                {
                    printf("用法: /register <用户名> <密码>\n");
                }
            }
            else if (strcmp(command, "login") == 0)
            {
                char *username = strtok(NULL, " ");
                char *password = strtok(NULL, " ");

                if (username && password)
                {
                    login_user(username, password);
                }
                else
                {
                    printf("用法: /login <用户名> <密码>\n");
                }
            }
            else if (strcmp(command, "logout") == 0)
            {
                logout_user();
            }
            else if (strcmp(command, "private") == 0)
            {
                char *to = strtok(NULL, " ");
                char *content = strtok(NULL, "");

                if (to && content)
                {
                    send_private_message(to, content);
                }
                else
                {
                    printf("用法: /private <用户名> <消息>\n");
                }
            }
            else if (strcmp(command, "group") == 0)
            {
                char *content = strtok(NULL, "");

                if (content)
                {
                    send_group_message(content);
                }
                else
                {
                    printf("用法: /group <消息>\n");
                }
            }
            else if (strcmp(command, "list") == 0)
            {
                request_online_list();
            }
            else if (strcmp(command, "help") == 0)
            {
                show_help();
            }
            else if (strcmp(command, "exit") == 0)
            {
                // 发送退出消息
                if (is_authenticated)
                {
                    message_t msg;
                    memset(&msg, 0, sizeof(msg));
                    msg.type = MSG_TYPE_EXIT;
                    strncpy(msg.username, g_username, MAX_USERNAME_LEN);
                    send_message(&msg);
                }

                printf("退出聊天室\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            }
            else
            {
                printf("未知命令: %s\n", command);
                printf("输入 /help 查看可用命令\n");
            }
        }
        else
        {
            // 非命令输入，作为群聊消息发送
            send_group_message(input);
        }
    }
}

// 格式化显示在线用户列表
void format_online_users(const char *raw_list)
{
    printf("\n=== 在线用户列表 ===\n");

    if (strlen(raw_list) == 0)
    {
        printf("目前没有其他在线用户\n");
        return;
    }

    char list_copy[BUFFER_SIZE];
    strncpy(list_copy, raw_list, BUFFER_SIZE - 1);
    list_copy[BUFFER_SIZE - 1] = '\0';

    char *token = strtok(list_copy, ",");
    int count = 0;

    while (token != NULL)
    {
        printf("%2d. %s\n", ++count, token);
        token = strtok(NULL, ",");
    }

    printf("===================\n");
}

// 接收服务器消息的线程函数
void *receive_messages(void *arg)
{
    message_t msg;

    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        ssize_t bytes_received = read(sockfd, &msg, sizeof(msg));

        if (bytes_received <= 0)
        {
            printf("与服务器的连接已断开\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        switch (msg.type)
        {
        case MSG_TYPE_SYSTEM:
            printf("\n[系统消息] %s\n", msg.content);

            // 处理认证结果
            if (strstr(msg.content, "登录成功") != NULL ||
                strstr(msg.content, "欢迎") != NULL)
            {
                is_authenticated = true;
                // 只有在登录成功时才设置用户名
                if (strlen(msg.username) > 0)
                {
                    strncpy(g_username, msg.username, MAX_USERNAME_LEN);
                }
                printf("=== 认证成功！您现在可以聊天了 ===\n");
            }
            // 处理登录失败
            else if (strstr(msg.content, "登录失败") != NULL ||
                     strstr(msg.content, "失败") != NULL)
            {
                is_authenticated = false;
                memset(g_username, 0, MAX_USERNAME_LEN);
                printf("认证失败，请检查用户名和密码\n");
            }
            // 处理退出登录
            else if (strstr(msg.content, "退出登录") != NULL ||
                     strstr(msg.content, "已退出") != NULL)
            {
                is_authenticated = false;
                memset(g_username, 0, MAX_USERNAME_LEN);
                printf("您已退出登录\n");
            }
            break;

        case MSG_TYPE_PRIVATE:
            printf("\n[私聊来自 %s] %s\n", msg.from, msg.content);
            break;

        case MSG_TYPE_GROUP:
            printf("\n[群聊 %s] %s\n", msg.from, msg.content);
            break;

        case MSG_TYPE_ONLINE_LIST:
            format_online_users(msg.content);
            break;

        default:
            printf("\n[未知消息类型: %d] %s\n", msg.type, msg.content);
            break;
        }

        // 重新显示提示符
        if (is_authenticated)
        {
            printf("[%s] > ", g_username);
        }
        else
        {
            printf("[未登录] > ");
        }
        fflush(stdout);
    }

    return NULL;
}

int main()
{
    struct sockaddr_in server_addr;

    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("无效的服务器地址");
        exit(EXIT_FAILURE);
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("连接服务器失败");
        exit(EXIT_FAILURE);
    }

    printf("已连接到聊天服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("输入 /help 查看可用命令\n");

    // 创建接收消息的线程
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0)
    {
        perror("创建接收线程失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 分离线程，使其在退出时自动释放资源
    pthread_detach(recv_thread);

    // 处理用户输入
    process_input();

    // 关闭套接字
    close(sockfd);

    return 0;
}