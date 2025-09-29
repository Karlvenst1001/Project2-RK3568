#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"
#include "thread_pool.h"
#include "client_list.h"
#include "message_handler.h"

int create_server_socket(int port); // 创建服务器套接字
void handle_client(void *arg);      // 处理客户端请求

#endif