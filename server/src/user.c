#include "user.h"

bool register_user(char *username, char *password)
{
    // 首先检查用户是否已存在
    int check_fd = open("users.txt", O_RDONLY);
    if (check_fd != -1)
    {
        char line[BUFFER_SIZE];
        ssize_t bytes_read;

        // 逐行读取文件，检查用户名是否已存在
        while ((bytes_read = read(check_fd, line, sizeof(line) - 1)) > 0)
        {
            line[bytes_read] = '\0'; // 确保字符串以null结尾

            // 将文件内容按行分割
            char *saveptr;
            char *token = strtok_r(line, "\n", &saveptr);

            while (token != NULL)
            {
                // 提取用户名部分（冒号前的部分）
                char *colon_pos = strchr(token, ':');
                if (colon_pos != NULL)
                {
                    // 计算用户名的长度
                    size_t username_len = colon_pos - token;

                    // 比较用户名
                    if (strncmp(token, username, username_len) == 0 &&
                        strlen(username) == username_len)
                    {
                        close(check_fd);
                        printf("Username already exists\n");
                        return false;
                    }
                }

                token = strtok_r(NULL, "\n", &saveptr);
            }
        }
        close(check_fd);
    }

    // 如果用户不存在，则创建/追加到文件
    int fd = open("users.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1)
    {
        printf("Error opening file\n");
        return false;
    }

    char buffer[BUFFER_SIZE];
    int len = snprintf(buffer, BUFFER_SIZE, "%s:%s\n", username, password);
    if (len >= BUFFER_SIZE)
    {
        printf("Username or password too long\n");
        close(fd);
        return false;
    }

    if (write(fd, buffer, len) == -1)
    {
        printf("Error writing to file\n");
        close(fd);
        return false;
    }

    if (close(fd) == -1)
    {
        printf("Error closing file\n");
        return false;
    }

    printf("User registered successfully\n");
    return true;
}

bool login_user(char *username, char *password)
{
    int fd = open("users.txt", O_RDONLY);
    if (fd == -1)
    {
        printf("Error opening file\n");
        return false;
    }

    char line[BUFFER_SIZE];
    ssize_t bytes_read;
    bool found = false;

    // 逐行读取文件，检查用户名和密码是否匹配
    while ((bytes_read = read_line(fd, line, sizeof(line))) > 0)
    {
        // 查找冒号分隔符
        char *colon = strchr(line, ':');
        if (colon == NULL)
        {
            continue; // 跳过无效行
        }

        // 分割用户名和密码
        *colon = '\0';
        char *user = line;
        char *pass = colon + 1;

        // 去除可能的换行符
        char *newline = strchr(pass, '\n');
        if (newline != NULL)
        {
            *newline = '\0';
        }

        // 比较用户名和密码
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0)
        {
            found = true;
            break;
        }
    }

    close(fd);
    return found;
}

// 辅助函数：读取一行
ssize_t read_line(int fd, char *buffer, size_t size)
{
    char ch;
    size_t idx = 0;

    while (idx < size - 1)
    {
        ssize_t n = read(fd, &ch, 1);
        if (n <= 0)
        {
            break; // 文件结束或错误
        }

        if (ch == '\n')
        {
            break; // 行结束
        }

        buffer[idx++] = ch;
    }

    buffer[idx] = '\0';
    return idx;
}

bool store_message(const char *username, const char *data)
{
    char filename[BUFFER_SIZE];
    // 确保文件名不会溢出
    int written = snprintf(filename, sizeof(filename), "%s.txt", username);
    if (written >= sizeof(filename))
    {
        printf("Warning: Filename truncated in store_message\n");
        filename[sizeof(filename) - 1] = '\0';
    }

    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1)
    {
        printf("Error opening file %s\n", filename);
        return false;
    }

    int index = 0;
    while (data[index] != '\0')
    {
        if (write(fd, &data[index], 1) == -1)
        {
            printf("Error writing to file %s\n", filename);
            close(fd);
            return false;
        }
        index++;
    }

    close(fd);
    return true;
}

bool read_history_messages(const char *username, int sockfd)
{
    if (username == NULL || sockfd < 0)
    {
        printf("Error: Invalid parameters in read_history_messages\n");
        return false;
    }

    char filename[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "%s.txt", username);

    printf("Reading history from file: %s for user: %s\n", filename, username);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        // 文件不存在，可能还没有消息
        printf("History file not found: %s\n", filename);
        return false;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    bool has_more_data = true;

    printf("send historical messages to the user: %s\n", username);

    while (has_more_data)
    {
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);

        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';

            // 创建消息并发送
            message_t history_msg;
            memset(&history_msg, 0, sizeof(history_msg));
            history_msg.type = MSG_TYPE_SYSTEM;
            strcpy(history_msg.from, "Server");
            strcpy(history_msg.to, username);

            // 安全地构建消息内容
            if (strcmp(username, "ALL") == 0)
            {
                strncpy(history_msg.content, "[Group chat history] ", BUFFER_SIZE - 1); // 群聊历史
            }
            else
            {
                strncpy(history_msg.content, "[Private chat history] ", BUFFER_SIZE - 1); // 私聊历史
            }

            // 确保不会缓冲区溢出
            size_t prefix_len = strlen(history_msg.content);
            size_t remaining = BUFFER_SIZE - prefix_len - 1;
            strncat(history_msg.content, buffer, remaining);

            // 发送消息
            ssize_t send_result = write(sockfd, &history_msg, sizeof(history_msg));
            if (send_result == -1)
            {
                perror("send historical messages filed");
                close(fd);
                return false;
            }

            // 添加小延迟，避免发送过快
            usleep(10000); // 10ms
        }
        else if (bytes_read == 0)
        {
            // 文件结束
            has_more_data = false;
        }
        else
        {
            // 读取错误
            perror("read historical messages filed");
            close(fd);
            return false;
        }
    }

    close(fd);

    // 发送结束标记
    message_t end_msg;
    memset(&end_msg, 0, sizeof(end_msg));
    end_msg.type = MSG_TYPE_SYSTEM;
    strcpy(end_msg.from, "Server");
    strcpy(end_msg.to, username);

    if (strcmp(username, "ALL") == 0)
    {
        strcpy(end_msg.content, "[End of group chat history records]");
    }
    else
    {
        strcpy(end_msg.content, "[End of private chat history records]");
    }

    write(sockfd, &end_msg, sizeof(end_msg));

    printf("send historical messages success: %s\n", username);
    return true;
}