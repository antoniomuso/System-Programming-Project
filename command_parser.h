//
// Created by anotoniomusolino on 02/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H
#define SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H

#define MAX_OPTION_LEN 20
#define REALLOC_INC_SIZE 4

#define MAX_HTTP_FIELD_LEN 60

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
    int is_request ; // if is_request < 0 there are a error during parsing.
    int code_respoce; // defined only if is_request is 0
    char * type_req;
    char * url ;
    char * protocol_type;
    char * authorization; //example, Authorization: Basic am9lYjp4eDEyMw==
    char * user_agent; //example, User-Agent:
    int content_length; //example, Content-Length: 45033
    char * content_type;
    char * connection;
    char * pointer_to_free;
};
typedef struct http_header http_header;



char* get_command_value (char command[], options options);
options options_parse (int argc, char *argv[], command_arc command_list[], int len_comm);

options parse_file(char *name, command_arc cmd_arc[], int arc_len);
void free_options(options opt);
http_header parse_http_header_request (const char* data, int data_len);
void free_http_header(http_header http_h);
#endif //SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H


