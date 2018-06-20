//
// Created by anotoniomusolino on 18/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
#define SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
int execCommand(int socket, const char * command, const char * args);
void send_file (int socket, char * url);
int log_write(char *cli_addr, char *user_id, char *username, char *request, int return_code, int bytes_sent);
#endif //SYSTEM_PROGRAMMING_PROJECT_OPERATIONS_H
