#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <netdb.h>
#include "cJSON.h"

// 服务器配置
#define IP_LEN 16
#define SERVER_PORT 8888
#define MAX_CLIENTS 15
#define BUFFER_SIZE 1024
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_ACTION_LEN 20

// 客户端状态枚举
typedef enum
{
    CLIENT_STATE_DISCONNECTED = 0, // 已断开连接
    CLIENT_STATE_CONNECTED,        // 已连接但未认证
    CLIENT_STATE_AUTHENTICATED     // 已连接并认证
} client_state_t;

// 消息类型
typedef enum
{
    MSG_TYPE_AUTH = 1,    // 认证消息
    MSG_TYPE_PRIVATE,     // 私聊消息
    MSG_TYPE_GROUP,       // 群聊消息
    MSG_TYPE_ONLINE_LIST, // 请求在线列表
    MSG_TYPE_SYSTEM,      // 系统消息
    MSG_TYPE_HISTORY,     // 请求历史消息
    MSG_TYPE_WEATHER,     // 请求天气
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

// 客户端信息结构
typedef struct client_info
{
    int sockfd;                      // 客户端套接字
    char ip[IP_LEN];                 // IP地址
    char username[MAX_USERNAME_LEN]; // 用户名
    client_state_t state;            // 客户端状态
    pthread_t thread_id;             // 处理该客户端的线程ID
    time_t connect_time;             // 连接时间
    time_t last_activity;            // 最后活动时间
    struct client_info *next;        // 指向下一个客户端
} client_info_t;

#endif