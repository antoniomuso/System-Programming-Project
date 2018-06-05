//
// Created by anotoniomusolino on 02/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H
#define SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H

#define MAX_OPTION_LEN 20
#define REALLOC_INC_SIZE 4

struct command_arc {
    char name[MAX_OPTION_LEN];
    char type[8]; // Accepted types are: int, str, null, float
};


struct command {
    char name[MAX_OPTION_LEN];
    char value[MAX_OPTION_LEN];
};



typedef struct command command;
typedef struct command_arc command_arc;

struct options {
    command* commands;
    int comm_len;
};
typedef struct options options;

struct http_header {
    char * type_req;
    char * url;
    char * protocol_type;
    char * authorization; //example, Authorization: Basic am9lYjp4eDEyMw==
    char * user_agent; //example, User-Agent:
    int content_length; //example, Content-Length: 45033
};
typedef struct http_header http_header;



char* get_command_value (char command[], options options);
options options_parse (int argc, char *argv[], command_arc command_list[], int len_comm);

command extract_command(char *string);
options parse_file(char *name, command_arc cmd_arc[], int arc_len);

http_header parse_http_request (char* data, int data_len);
#endif //SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H


