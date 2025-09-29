#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include "common.h"

extern client_info_t *client_list_head; // 使用 extern 声明
extern pthread_mutex_t client_list_mutex;

client_info_t *add_client(int sockfd, const char *ip); // 添加客户端
bool remove_client(int sockfd);                        // 移除客户端
bool remove_client_by_username(const char *username);
client_info_t *find_client_by_sockfd(int sockfd);
client_info_t *find_client_by_sockfd_nosock(int sockfd);
client_info_t *find_client_by_username(const char *username);
client_info_t *find_client_by_ip(const char *ip);
void update_client_username(int sockfd, const char *username);
void update_client_state(int sockfd, client_state_t state);
void update_client_activity(int sockfd);
int get_connected_count(void);
int get_authenticated_count(void);
void print_client_list(void);
void clear_client_list(void);
void cleanup_inactive_clients(int timeout_seconds);

#endif