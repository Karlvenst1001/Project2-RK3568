#include "network.h"

// 创建服务器套接字
int create_server_socket(int port)
{
    int server_fd;
    struct sockaddr_in server_addr;

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    // 设置套接字选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        return -1;
    }

    // 绑定地址和端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定套接字信息
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed:");
        return -1;
    }

    // 监听
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        perror("Listen failed:");
        return -1;
    }

    printf("Server is open, port on %d\n", port);

    // 返回服务器套接字
    return server_fd;
}

// 处理客户端连接
void handle_client(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg); // 释放动态分配的内存

    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    // 获取客户端地址
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    char client_ip[IP_LEN];
    strcpy(client_ip, inet_ntoa(client_addr.sin_addr));

    // 将客户端添加到链表
    client_info_t *client = add_client(client_sock, client_ip);
    if (client == (client_info_t *)NULL)
    {
        printf("Failed to add client to list\n");
        close(client_sock);
        return;
    }

    printf("New client connected: %s\n", client_ip);

    // 客户端消息处理循环
    while (1)
    {
        memset(&msg, 0, sizeof(message_t));
        ssize_t read_msg_len = recv(client_sock, &msg, sizeof(message_t), 0);
        if (read_msg_len == -1)
        {
            perror("read failed:");
            break;
        }
        else if (read_msg_len == 0)
        {
            printf("Client disconnected: %s\n", client_ip);
            break;
        }
        else
        {
            // printf("Received message from %s\n", client_ip);
        }

        update_client_activity(client_sock);

        // 根据消息类型处理
        switch (msg.type)
        {
        case MSG_TYPE_AUTH:
            printf("Auth request from %s\n", client_ip);
            if (process_auth_message(client_sock, &msg) == false)
            {
                printf("Auth failed for %s\n", client_ip);
            }
            break;

        case MSG_TYPE_PRIVATE:
            printf("Private message from %s to %s\n", msg.from, msg.to);
            process_private_message(&msg);
            break;

        case MSG_TYPE_GROUP:
            printf("Group message from %s\n", msg.from);
            process_group_message(&msg);
            break;

        case MSG_TYPE_ONLINE_LIST:
            send_online_list(client_sock);
            break;

        case MSG_TYPE_HISTORY:
            printf("History request from %s\n", msg.from);
            send_history(&msg);
            break;

        case MSG_TYPE_WEATHER:
            printf("Weather request from %s\n", msg.from);
            obtain_weather(&msg);
            break;

        case MSG_TYPE_EXIT:
            printf("Exit request from %s\n", msg.from);
            if (remove_client(client_sock) == false)
            {
                printf("Failed to remove client from list\n");
            }

            close(client_sock);
            break;

        default:
            printf("Unknown message type from %s\n", client_ip);
            continue;
        }
    }

    // 客户端断开连接，从链表中移除
    if (remove_client(client_sock) == false)
    {
        printf("Failed to remove client from list\n");
    }

    close(client_sock);
}