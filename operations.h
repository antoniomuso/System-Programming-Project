//
// Created by anotoniomusolino on 18/06/18.
//
#include "command_parser.h"

#ifndef SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
#define SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
int exec_command(int socket, const char * command, const char * args, http_header http_h, char * address);
void send_file (int socket, http_header http_h, char * address);
int http_log (http_header h_request, char * h_response, char * client_address, int no_name);
void put_file (int clientfd, http_header http_h, char * address, char * buffer, const int BUFF_READ_LEN,int header_len, int data_read);
#endif //SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
