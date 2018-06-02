//
// Created by anotoniomusolino on 02/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H
#define SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H

#define MAX_OPTION_LEN 20
#define REALLOC_INC_SIZE 4

struct command_arc {
    char name[MAX_OPTION_LEN];
    char type[8]; // Accept type are: int, str, null, float
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


char* get_command_value (char command[], options options);
options options_parse (int argc, char *argv[], command_arc command_list[], int len_comm);

#endif //SYSTEM_PROGRAMMING_PROJECT_COMMAND_PARSER_H


