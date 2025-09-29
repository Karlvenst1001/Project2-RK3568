#include "client_list.h"

client_info_t *client_list_head = NULL;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

// 添加客户端到链表
client_info_t *add_client(int sockfd, const char *ip)
{
    pthread_mutex_lock(&client_list_mutex);

    // 检查是否已存在该客户端
    client_info_t *exist = NULL;
    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (current->sockfd == sockfd)
        {
            exist = current;
            break;
        }
        current = current->next;
    }
    if (exist != NULL)
    {
        pthread_mutex_unlock(&client_list_mutex);
        return exist;
    }

    // 创建新客户端节点
    client_info_t *new_client = (client_info_t *)malloc(sizeof(client_info_t));
    if (new_client == NULL)
    {
        pthread_mutex_unlock(&client_list_mutex);
        perror("new_client malloc failed:");
        return NULL;
    }

    // 初始化客户端信息
    new_client->sockfd = sockfd;
    strncpy(new_client->ip, ip, IP_LEN);
    new_client->username[0] = '\0';
    new_client->state = CLIENT_STATE_CONNECTED;
    new_client->thread_id = pthread_self();
    new_client->connect_time = time(NULL);
    new_client->last_activity = time(NULL);
    new_client->next = client_list_head;

    client_list_head = new_client;

    pthread_mutex_unlock(&client_list_mutex);

    printf("Client added to list successed\n");
    return new_client;
}

// 通过套接字移除客户端
bool remove_client(int sockfd)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    client_info_t *prev = NULL;
    bool found = false;

    while (current != NULL)
    {
        if (current->sockfd == sockfd)
        {
            if (prev == NULL)
            {
                client_list_head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            printf("Client %s (socket %d) removed from list\n",
                   current->ip, current->sockfd);
            free(current);
            found = true;
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return found;
}

// 通过用户名移除客户端
bool remove_client_by_username(const char *username)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    client_info_t *prev = NULL;
    bool found = false;

    while (current != NULL)
    {
        if (strcmp(current->username, username) == 0)
        {
            if (prev == NULL)
            {
                client_list_head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            printf("Client %s (username %s) removed from list\n",
                   current->ip, current->username);
            free(current);
            found = true;
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return found;
}

// 通过套接字查找客户端
client_info_t *find_client_by_sockfd(int sockfd)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (current->sockfd == sockfd)
        {
            pthread_mutex_unlock(&client_list_mutex);
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return NULL;
}

// 通过套接字查找客户端
client_info_t *find_client_by_sockfd_nosock(int sockfd)
{
    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (current->sockfd == sockfd)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 通过用户名查找客户端
client_info_t *find_client_by_username(const char *username)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (strcmp(current->username, username) == 0)
        {
            pthread_mutex_unlock(&client_list_mutex);
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return NULL;
}

// 通过IP地址查找客户端
client_info_t *find_client_by_ip(const char *ip)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (strcmp(current->ip, ip) == 0)
        {
            pthread_mutex_unlock(&client_list_mutex);
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return NULL;
}

// 更新客户端用户名
void update_client_username(int sockfd, const char *username)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *client = find_client_by_sockfd_nosock(sockfd);
    if (client != NULL)
    {
        strncpy(client->username, username, MAX_USERNAME_LEN);
        client->username[MAX_USERNAME_LEN - 1] = '\0'; // 确保字符串终止
        printf("Client %s username set to %s\n", client->ip, client->username);
    }

    pthread_mutex_unlock(&client_list_mutex);
}

// 更新客户端状态
void update_client_state(int sockfd, client_state_t state)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *client = find_client_by_sockfd_nosock(sockfd);
    if (client != NULL)
    {
        client->state = state;
        printf("Client %s state changed to %d\n", client->ip, state);
    }

    pthread_mutex_unlock(&client_list_mutex);
}

// 更新客户端最后活动时间
void update_client_activity(int sockfd)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *client = find_client_by_sockfd_nosock(sockfd);
    if (client != NULL)
    {
        client->last_activity = time(NULL);
    }

    pthread_mutex_unlock(&client_list_mutex);
}

// 获取已连接客户端数量
int get_connected_count(void)
{
    pthread_mutex_lock(&client_list_mutex);

    int count = 0;
    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (current->state != CLIENT_STATE_DISCONNECTED)
        {
            count++;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return count;
}

// 获取已认证客户端数量
int get_authenticated_count(void)
{
    pthread_mutex_lock(&client_list_mutex);

    int count = 0;
    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        if (current->state == CLIENT_STATE_AUTHENTICATED)
        {
            count++;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&client_list_mutex);
    return count;
}

// 打印客户端列表
void print_client_list(void)
{
    pthread_mutex_lock(&client_list_mutex);

    printf("\n=== Client List ===\n");
    printf("%-15s %-20s %-15s %-12s %-8s\n",
           "Socket", "IP", "Username", "State", "Active(s)");
    printf("------------------------------------------------------------\n");

    client_info_t *current = client_list_head;
    time_t now = time(NULL);

    while (current != NULL)
    {
        const char *state_str;
        switch (current->state)
        {
        case CLIENT_STATE_DISCONNECTED:
            state_str = "DISCONNECTED";
            break;
        case CLIENT_STATE_CONNECTED:
            state_str = "CONNECTED";
            break;
        case CLIENT_STATE_AUTHENTICATED:
            state_str = "AUTHENTICATED";
            break;
        default:
            state_str = "UNKNOWN";
            break;
        }

        int active_seconds = (int)difftime(now, current->last_activity);
        printf("%-15d %-20s %-15s %-12s %-8d\n",
               current->sockfd, current->ip, current->username,
               state_str, active_seconds);

        current = current->next;
    }

    printf("Total: %d clients (%d authenticated)\n",
           get_connected_count(), get_authenticated_count());
    printf("============================================================\n");

    pthread_mutex_unlock(&client_list_mutex);
}

// 清空客户端链表
void clear_client_list(void)
{
    pthread_mutex_lock(&client_list_mutex);

    client_info_t *current = client_list_head;
    while (current != NULL)
    {
        client_info_t *next = current->next;

        // 关闭套接字（如果仍然打开）
        if (current->sockfd >= 0)
        {
            close(current->sockfd);
        }

        free(current);
        current = next;
    }

    client_list_head = NULL;
    printf("Client list cleared\n");

    pthread_mutex_unlock(&client_list_mutex);
}

// 清理不活跃的客户端
void cleanup_inactive_clients(int timeout_seconds)
{
    pthread_mutex_lock(&client_list_mutex);

    time_t now = time(NULL);
    client_info_t *current = client_list_head;
    client_info_t *prev = NULL;

    while (current != NULL)
    {
        double inactive_seconds = difftime(now, current->last_activity);

        if (inactive_seconds > timeout_seconds)
        {
            printf("Cleaning up inactive client: %s (inactive for %.0f seconds)\n",
                   current->ip, inactive_seconds);

            // 从链表中移除
            if (prev == NULL)
            {
                client_list_head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            // 关闭套接字并释放内存
            client_info_t *to_remove = current;
            current = current->next;

            if (to_remove->sockfd >= 0)
            {
                close(to_remove->sockfd);
            }

            free(to_remove);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }

    pthread_mutex_unlock(&client_list_mutex);
}