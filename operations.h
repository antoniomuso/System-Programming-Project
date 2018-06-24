#include "command_parser.h"

#ifndef SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
#define SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
void m_sleep (unsigned int time);
int Send(int socket, const void * buff, int size, int flag);
int exec_command(int socket, const char * command, const char * args, http_header http_h, char * address);
void send_file (int socket, http_header http_h, char * address);
void send_file_chipher (int socket, http_header http_h, unsigned int address, char * conv_address);
int http_log (http_header h_request, char * h_response, char * client_address, int no_name);
void put_file (int clientfd, http_header http_h, char * address, char * buffer, const int BUFF_READ_LEN,int header_len, int data_read);
#endif //SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
