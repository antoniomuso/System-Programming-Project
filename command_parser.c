//
// Created by anotoniomusolino on 31/05/18.
//
#include <stdlib.h>
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

/**
 *
 * @param command Command String
 * @param options Commands data struct
 * @return NULL if command isn't contain insiede options,
 *         Pointer to a string that it contain the command value.
 */
char* get_command_value (char command[], options options) {
    for (int i = 0; i < options.comm_len; i++) {
        if (strcmp(options.commands[i].name, command) == 0) return options.commands[i].value;
    }
    return NULL;
}

/**
 *
 * @param argc
 * @param argv
 * @param command_list List of Archetype to pars parameters
 * @param len_comm Len of command_list
 * @return Return Options struct, after use is request free of Options.commands.
 */

options options_parse (int argc, char *argv[], command_arc command_list[], int len_comm) {

    int pos = 0;
    int len = 6;
    command* comm = calloc(sizeof(command), len);



    for (int i = 1; i < argc ; i++) {
        for (int j = 0; j < len_comm; j++) {

            if (strcmp(argv[i], command_list[j].name ) == 0) {

                if (pos >= len) {
                    len += REALLOC_INC_SIZE;
                    comm = (command*) realloc(comm, sizeof(command) * len);
                    if (comm == NULL) {
                        fprintf(stderr, "Options realloc failed");
                        exit(EXIT_FAILURE);
                    }
                }

                if (strlen(argv[i]) > MAX_OPTION_LEN) {
                    fprintf(stderr, "Command %s exceeds the maximum command length", argv[i]);
                    exit(EXIT_FAILURE);
                }

                strcpy(comm[pos].name,argv[i]);

                if ( strcmp(command_list[j].type, "null") == 0 ) {
                    command c ;
                    strcpy(comm[pos].value,"NOT VALUE");
                    pos++;
                    break;
                }
                // Passo all'indice dell'input
                if (++i >= argc) {
                    fprintf(stderr, "Iput for command %s not passed", argv[i-1]);
                    exit(EXIT_FAILURE);
                }

                // Controllo che l'input non superi la lunghezza massima
                if (strlen(argv[i]) > MAX_OPTION_LEN) {
                    fprintf(stderr, "Command input %s exceeds the maximum command length", argv[i]);
                    exit(EXIT_FAILURE);
                }

                if ( strcmp(command_list[j].type, "int") == 0 ) {
                    sprintf( comm[pos].value,"%d",atoi(argv[i]));
                    pos++;
                    break;
                }

                if ( strcmp(command_list[j].type, "float") == 0 ) {
                    sprintf( comm[pos].value,"%f",atof(argv[i]));
                    pos++;
                    break;
                }

                if ( strcmp(command_list[j].type, "str") == 0 ) {
                    strcpy(comm[pos].value, argv[i]);
                    pos++;
                    break;
                }


            }

        }
    }

    options option;
    option.comm_len = pos;
    option.commands = comm;

    return option;
}





