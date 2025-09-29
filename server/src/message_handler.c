#include "message_handler.h"

// 处理认证消息
bool process_auth_message(int sockfd, message_t *msg)
{
    if (strlen(msg->action) == 0)
    {
        return false;
    }

    if (strcmp(msg->action, "register") == 0)
    {
        // 注册新用户
        if (register_user(msg->username, msg->password))
        {
            message_t response;
            memset(&response, 0, sizeof(response));
            response.type = MSG_TYPE_SYSTEM;
            strcpy(response.content, "Register success");
            write(sockfd, &response, sizeof(response));

            printf("register success\n");
            return true;
        }
        else
        {
            message_t response;
            memset(&response, 0, sizeof(response));
            response.type = MSG_TYPE_SYSTEM;
            strcpy(response.content, "Register failed");
            write(sockfd, &response, sizeof(response));
            return false;
        }
    }
    else if (strcmp(msg->action, "login") == 0)
    {
        // 登录
        if (login_user(msg->username, msg->password))
        {
            update_client_username(sockfd, msg->username);
            update_client_state(sockfd, CLIENT_STATE_AUTHENTICATED);

            message_t response;
            memset(&response, 0, sizeof(response));
            response.type = MSG_TYPE_SYSTEM;
            strcpy(response.content, "Login success");

            // 检查连接是否仍然有效
            if (write(sockfd, &response, sizeof(response)) == -1)
            {
                perror("Write failed");
                return false;
            }

            printf("login success\n");
            return true;
        }
        else
        {
            message_t response;
            memset(&response, 0, sizeof(response));
            response.type = MSG_TYPE_SYSTEM;
            strcpy(response.content, "Login failed");

            // 检查连接是否仍然有效
            if (write(sockfd, &response, sizeof(response)) == -1)
            {
                perror("Write failed");
                return false;
            }

            return false;
        }
    }
    else
    {
        return false;
    }
}

// 处理私聊消息
void process_private_message(message_t *msg)
{
    // 查找目标客户端
    client_info_t *receiver = find_client_by_username(msg->to);
    if (receiver == NULL || receiver->state != CLIENT_STATE_AUTHENTICATED)
    {
        printf("Target user %s not found or not authenticated\n", msg->to);
        return;
    }

    // 存储消息格式
    char buffer[BUFFER_SIZE + MAX_USERNAME_LEN + 10];
    int written = snprintf(buffer, sizeof(buffer), "[PRIVATE]%s->%s:%s\n", msg->from, msg->to, msg->content);
    if (written >= sizeof(buffer))
    {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    // 存储消息到接收方和发送方的历史记录
    store_message(msg->to, buffer);   // 接收方的历史
    store_message(msg->from, buffer); // 发送方的历史

    // 转发消息给接收方和发送方
    write(receiver->sockfd, msg, sizeof(message_t));

    client_info_t *sender = find_client_by_username(msg->from);
    if (sender != NULL && sender->state == CLIENT_STATE_AUTHENTICATED)
    {
        write(sender->sockfd, msg, sizeof(message_t));
    }

    printf("Private message delivered: %s -> %s\n", msg->from, msg->to);
}

// 处理群聊消息
// 处理群聊消息
void process_group_message(message_t *msg)
{
    // 首先验证消息有效性
    if (msg == NULL || strlen(msg->from) == 0)
    {
        printf("Error: Invalid group message\n");
        return;
    }

    printf("Processing group message from: %s\n", msg->from);

    // 存储消息到统一的 ALL 文件
    char buffer[BUFFER_SIZE + MAX_USERNAME_LEN + 10];
    int written = snprintf(buffer, sizeof(buffer), "[GROUP]%s:%s\n", msg->from, msg->content);
    if (written >= sizeof(buffer))
    {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    // 存储到 ALL 文件
    if (store_message("ALL", buffer) == false)
    {
        printf("Failed to store group message to ALL file\n");
    }

    // 广播给所有已认证的客户端（包括发送方自己）
    client_info_t *current = client_list_head;
    int send_count = 0;

    while (current != NULL)
    {
        if (current->state == CLIENT_STATE_AUTHENTICATED)
        {
            ssize_t result = write(current->sockfd, msg, sizeof(message_t));
            if (result == -1)
            {
                printf("Failed to send group message to %s\n", current->username);
            }
            else
            {
                send_count++;
            }
        }
        current = current->next;
    }

    printf("Group message from %s broadcasted to %d users\n", msg->from, send_count);
}

// 发送在线用户列表
void send_online_list(int sockfd)
{
    char list_buffer[BUFFER_SIZE] = "Online users:";
    client_info_t *current = client_list_head;

    while (current != NULL)
    {
        if (current->state == CLIENT_STATE_AUTHENTICATED)
        {
            strcat(list_buffer, current->username);
            strcat(list_buffer, ", ");
        }
        current = current->next;
    }

    current = find_client_by_sockfd(sockfd);
    if (current == NULL)
    {
        return (void)-1;
    }
    char *username = current->username;
    // 创建系统消息
    message_t response;
    memset(&response, 0, sizeof(response));
    response.type = MSG_TYPE_SYSTEM;
    strcpy(response.from, "Server");
    strcpy(response.to, username);
    strncpy(response.content, list_buffer, BUFFER_SIZE);

    // 发送列表
    write(sockfd, &response, sizeof(response));
}

void send_history(message_t *msg)
{
    printf("History request for: %s\n", msg->to);
    char *username = msg->to;

    // 查找发送方客户端
    client_info_t *sender = find_client_by_username(msg->from);
    if (sender == NULL)
    {
        printf("Error: Sender %s not found when requesting history\n", msg->from);
        return;
    }

    // 如果请求的是 ALL，则发送群聊历史
    if (strcmp(username, "ALL") == 0)
    {
        if (read_history_messages("ALL", sender->sockfd) == false)
        {
            printf("No group history found\n");

            // 发送空历史消息提示
            message_t no_history_msg;
            memset(&no_history_msg, 0, sizeof(no_history_msg));
            no_history_msg.type = MSG_TYPE_SYSTEM;
            strcpy(no_history_msg.from, "Server");
            strcpy(no_history_msg.to, msg->from);
            strcpy(no_history_msg.content, "暂无群聊历史记录");
            write(sender->sockfd, &no_history_msg, sizeof(no_history_msg));
        }
    }
    else
    {
        // 请求的是私聊历史
        if (read_history_messages(username, sender->sockfd) == false)
        {
            printf("No private history found for %s\n", username);

            // 发送空历史消息提示
            message_t no_history_msg;
            memset(&no_history_msg, 0, sizeof(no_history_msg));
            no_history_msg.type = MSG_TYPE_SYSTEM;
            strcpy(no_history_msg.from, "Server");
            strcpy(no_history_msg.to, msg->from);
            strcpy(no_history_msg.content, "暂无私聊历史记录");
            write(sender->sockfd, &no_history_msg, sizeof(no_history_msg));
        }
    }
}

void obtain_weather(message_t *msg)
{
    printf("Weather request for city: %s from user: %s\n", msg->content, msg->from);

    // 查找发送方客户端
    client_info_t *sender = find_client_by_username(msg->from);
    if (sender == NULL)
    {
        printf("Error: Sender %s not found when requesting weather\n", msg->from);
        return;
    }

    char *city = msg->content;
    if (strlen(city) == 0)
    {
        // 如果城市名为空，发送错误消息
        message_t error_msg;
        memset(&error_msg, 0, sizeof(error_msg));
        error_msg.type = MSG_TYPE_SYSTEM;
        strcpy(error_msg.from, "Server");
        strcpy(error_msg.to, msg->from);
        strcpy(error_msg.content, "请输入城市名称");
        write(sender->sockfd, &error_msg, sizeof(error_msg));
        return;
    }

    // 创建子进程获取天气数据
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("fork error\n");
        return;
    }
    else if (pid == 0)
    {
        // 子进程：获取天气数据
        get_weather_data(city, sender->sockfd);
        exit(0);
    }
    // 父进程继续，不等待子进程（异步处理）
}

// 新的函数：获取天气数据并发送给客户端
void get_weather_data(const char *city, int client_sock)
{
    // 把服务器的域名转换成IP地址
    struct hostent *he = gethostbyname("api.seniverse.com");
    if (he == NULL)
    {
        perror("gethostbyname failed");
        send_weather_error(client_sock, "无法解析天气服务器地址");
        return;
    }

    int weather_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (weather_fd == -1)
    {
        perror("socket failed");
        send_weather_error(client_sock, "创建天气查询连接失败");
        return;
    }

    // 连接天气服务器
    struct sockaddr_in ser_inf;
    memset(&ser_inf, 0, sizeof(ser_inf));
    ser_inf.sin_family = AF_INET;
    ser_inf.sin_port = htons(80);
    ser_inf.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));

    if (connect(weather_fd, (struct sockaddr *)&ser_inf, sizeof(ser_inf)) == -1)
    {
        perror("connect failed");
        close(weather_fd);
        send_weather_error(client_sock, "连接天气服务器失败");
        return;
    }

    // 构造HTTP请求
    char request[1024];
    snprintf(request, sizeof(request),
             "GET /v3/weather/daily.json?key=SAewqnjWlC7dvMLfL&location=%s"
             "&language=zh-Hans&unit=c&start=0&days=3 HTTP/1.1\r\n"
             "Host: api.seniverse.com\r\n"
             "Connection: close\r\n"
             "\r\n",
             city);

    if (write(weather_fd, request, strlen(request)) == -1)
    {
        perror("write request failed");
        close(weather_fd);
        send_weather_error(client_sock, "发送天气请求失败");
        return;
    }

    // 读取HTTP响应
    char response[8192]; // 增加缓冲区大小
    memset(response, 0, sizeof(response));

    // 读取整个响应
    ssize_t total_read = 0;
    ssize_t n;
    while ((n = read(weather_fd, response + total_read, sizeof(response) - total_read - 1)) > 0)
    {
        total_read += n;
        if (total_read >= sizeof(response) - 1)
            break;
    }

    if (n == -1)
    {
        perror("read response failed");
        close(weather_fd);
        send_weather_error(client_sock, "读取天气响应失败");
        return;
    }

    response[total_read] = '\0';
    close(weather_fd);

    // 提取JSON部分（跳过HTTP头部）
    char *json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL)
    {
        json_start = strstr(response, "\n\n");
        if (json_start != NULL)
            json_start += 2;
    }
    else
    {
        json_start += 4;
    }

    if (json_start == NULL)
    {
        send_weather_error(client_sock, "天气数据格式错误");
        return;
    }

    // 解析并格式化天气数据
    char formatted_weather[BUFFER_SIZE];
    if (parse_and_format_weather(json_start, formatted_weather, sizeof(formatted_weather)))
    {
        // 发送格式化后的天气数据给客户端
        send_formatted_weather(client_sock, formatted_weather);
    }
    else
    {
        send_weather_error(client_sock, "天气数据解析失败");
    }
}

// 解析并格式化天气数据
bool parse_and_format_weather(const char *json_str, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        printf("JSON解析失败: %s\n", cJSON_GetErrorPtr());
        return false;
    }

    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (results == NULL || !cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0)
    {
        printf("未找到results字段或results为空\n");
        cJSON_Delete(root);
        return false;
    }

    cJSON *first_result = cJSON_GetArrayItem(results, 0);
    if (first_result == NULL)
    {
        printf("results为空\n");
        cJSON_Delete(root);
        return false;
    }

    // 获取位置信息
    cJSON *location = cJSON_GetObjectItem(first_result, "location");
    char city_name[64] = "未知城市";
    if (location)
    {
        cJSON *name = cJSON_GetObjectItem(location, "name");
        if (name && cJSON_IsString(name))
        {
            strncpy(city_name, name->valuestring, sizeof(city_name) - 1);
        }
    }

    // 获取最后更新时间
    cJSON *last_update = cJSON_GetObjectItem(first_result, "last_update");
    char update_time[64] = "未知时间";
    if (last_update && cJSON_IsString(last_update))
    {
        strncpy(update_time, last_update->valuestring, sizeof(update_time) - 1);
    }

    // 开始构建格式化输出
    snprintf(output, output_size, "=== %s天气预报 ===\n更新时间: %s\n", city_name, update_time);

    // 获取每日天气
    cJSON *daily = cJSON_GetObjectItem(first_result, "daily");
    if (daily && cJSON_IsArray(daily))
    {
        int size = cJSON_GetArraySize(daily);
        for (int i = 0; i < size; i++)
        {
            cJSON *day = cJSON_GetArrayItem(daily, i);
            if (day)
            {
                char date[32] = "未知日期";
                char text_day[32] = "未知";
                char text_night[32] = "未知";
                char high[16] = "未知";
                char low[16] = "未知";
                char wind_speed[16] = "未知";
                char humidity[16] = "未知";
                char precip[32] = "未知";
                char rainfall[32] = "未知";

                cJSON *date_json = cJSON_GetObjectItem(day, "date");
                cJSON *text_day_json = cJSON_GetObjectItem(day, "text_day");
                cJSON *text_night_json = cJSON_GetObjectItem(day, "text_night");
                cJSON *high_json = cJSON_GetObjectItem(day, "high");
                cJSON *low_json = cJSON_GetObjectItem(day, "low");
                cJSON *wind_speed_json = cJSON_GetObjectItem(day, "wind_speed");
                cJSON *humidity_json = cJSON_GetObjectItem(day, "humidity");
                cJSON *precip_json = cJSON_GetObjectItem(day, "precip");
                cJSON *rainfall_json = cJSON_GetObjectItem(day, "rainfall");

                if (date_json && cJSON_IsString(date_json))
                    strncpy(date, date_json->valuestring, sizeof(date) - 1);
                if (text_day_json && cJSON_IsString(text_day_json))
                    strncpy(text_day, text_day_json->valuestring, sizeof(text_day) - 1);
                if (text_night_json && cJSON_IsString(text_night_json))
                    strncpy(text_night, text_night_json->valuestring, sizeof(text_night) - 1);
                if (high_json && cJSON_IsString(high_json))
                    strncpy(high, high_json->valuestring, sizeof(high) - 1);
                if (low_json && cJSON_IsString(low_json))
                    strncpy(low, low_json->valuestring, sizeof(low) - 1);
                if (wind_speed_json && cJSON_IsString(wind_speed_json))
                    strncpy(wind_speed, wind_speed_json->valuestring, sizeof(wind_speed) - 1);
                if (humidity_json && cJSON_IsString(humidity_json))
                    strncpy(humidity, humidity_json->valuestring, sizeof(humidity) - 1);
                if (precip_json && cJSON_IsString(precip_json))
                    strncpy(precip, precip_json->valuestring, sizeof(precip) - 1);
                if (rainfall_json && cJSON_IsString(rainfall_json))
                    strncpy(rainfall, rainfall_json->valuestring, sizeof(rainfall) - 1);

                // 添加到输出
                char day_info[512];
                snprintf(day_info, sizeof(day_info),
                         "日期: %s\n"
                         "  白天: %s, 夜间: %s\n"
                         "  温度: %s°C ~ %s°C\n"
                         "  风力: %skm/h, 湿度: %s%%\n"
                         "  降雨概率: %s%%, 降雨量: %smm\n"
                         "------------------------\n",
                         date, text_day, text_night, low, high, wind_speed, humidity, precip, rainfall);

                // 检查是否还有空间
                if (strlen(output) + strlen(day_info) < output_size - 1)
                {
                    strcat(output, day_info);
                }
                else
                {
                    // 输出缓冲区不足，添加省略号
                    strcat(output, "...\n");
                    break;
                }
            }
        }
    }

    cJSON_Delete(root);
    return true;
}

// 发送格式化后的天气数据给客户端
void send_formatted_weather(int client_sock, const char *weather_text)
{
    message_t weather_msg;
    memset(&weather_msg, 0, sizeof(weather_msg));
    weather_msg.type = MSG_TYPE_WEATHER;
    strcpy(weather_msg.from, "Server");
    strcpy(weather_msg.to, "Weather");

    // 确保不会缓冲区溢出
    strncpy(weather_msg.content, weather_text, BUFFER_SIZE - 1);
    weather_msg.content[BUFFER_SIZE - 1] = '\0';

    if (write(client_sock, &weather_msg, sizeof(weather_msg)) == -1)
    {
        perror("send formatted weather failed");
    }
    else
    {
        printf("Formatted weather data sent to client successfully\n");
    }
}

// 发送天气错误信息
void send_weather_error(int client_sock, const char *error_msg)
{
    message_t error_response;
    memset(&error_response, 0, sizeof(error_response));
    error_response.type = MSG_TYPE_SYSTEM;
    strcpy(error_response.from, "Server");
    strcpy(error_response.to, "Weather");
    strncpy(error_response.content, error_msg, BUFFER_SIZE - 1);

    write(client_sock, &error_response, sizeof(error_response));
}