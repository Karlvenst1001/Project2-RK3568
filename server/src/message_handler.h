#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "common.h"
#include "client_list.h"
#include "user.h"

bool process_auth_message(int sockfd, message_t *msg);
void process_private_message(message_t *msg);
void process_group_message(message_t *msg);
void send_online_list(int sockfd);
void send_history(message_t *msg);
void obtain_weather(message_t *msg);
void get_weather_data(const char *city, int client_sock);
bool parse_and_format_weather(const char *json_str, char *output, size_t output_size);
void send_formatted_weather(int client_sock, const char *weather_text);
void send_weather_error(int client_sock, const char *error_msg);

#endif