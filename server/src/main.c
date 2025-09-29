#include "common.h"
#include "network.h"
#include "thread_pool.h"
#include "client_list.h"

thread_pool_t *pool;
int server_fd = -1;

void signal_handler(int sig)
{
    printf("\nShutting down server...\n");

    thread_pool_destroy(pool);
    clear_client_list();
    exit(0);
}

int main()
{
    signal(SIGINT, signal_handler);


    // 创建服务器套接字
    server_fd = create_server_socket(SERVER_PORT);
    if (server_fd == -1)
    {
        printf("Failed to create server socket\n");
        return -1;
    }

    // 创建线程池
    pool = thread_pool_create(4);
    if (pool == NULL)
    {
        printf("Failed to create thread pool\n");
        close(server_fd);
        return -1;
    }

    printf("Server started successfully. Waiting for connections...\n");

    // 主循环接受客户端连接
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int *client_sockfd = malloc(sizeof(int));
        if (client_sockfd == NULL)
        {
            perror("Failed to allocate memory for client socket");
            continue;
        }

        *client_sockfd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_sockfd == -1)
        {
            perror("Failed to accept client");
            free(client_sockfd);
            continue;
        }

        if (!thread_pool_add_task(pool, handle_client, client_sockfd))
        {
            printf("Failed to add task to thread pool\n");
            close(*client_sockfd);
            free(client_sockfd);
        }
    }

    return 0;
}