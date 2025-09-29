#ifndef USER_H
#define USER_H

#include "common.h"

bool register_user(char *username, char *password);
bool login_user(char *username, char *password);
ssize_t read_line(int fd, char *buffer, size_t size);
bool store_message(const char *username, const char *data);
bool read_history_messages(const char *username, int sockfd);

#endif
