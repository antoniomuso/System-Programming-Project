//
// Created by anotoniomusolino on 31/05/18.
//
#include <stdlib.h>
#define MAX_OPTION_LEN 20
#define REALLOC_INC_SIZE 4

struct command_arc {
    char name[MAX_OPTION_LEN];
    char type[8]; // Accept type are: int, str, null
};


struct command {
    char name[MAX_OPTION_LEN];
    char value[MAX_OPTION_LEN];
};



typedef struct command command;
typedef struct command_arc command_arc;

struct options {
    command* commands;
    u_int comm_len;
};
typedef struct options options;


/**
 *
 * @param argc
 * @param argv
 * @param command_list List of Archetype to pars parameters
 * @param len_comm Len of command_list
 * @return Return Options struct, after use is request free of Options.commands.
 */

options options_parse (int argc, char *argv[], command_arc command_list[], int len_comm) {

    u_int pos = 0;
    u_int len = 6;
    command* comm = calloc(sizeof(command), len);



    for (int i = 1; i < argc ; i++) {
        for (int j = 0; j < len_comm; j++) {

            if (strcmp(argv[i], command_list[j].name ) == 0) {

                if (pos >= len) {
                    len += REALLOC_INC_SIZE;
                    comm = (command*) realloc(comm, sizeof(command) * len);
                    if (comm == NULL) exit(EXIT_FAILURE);
                }

                if (strlen(argv[i]) > MAX_OPTION_LEN) exit(EXIT_FAILURE);

                strcpy(comm[pos].name,argv[i]);

                if ( strcmp(command_list[j].type, "null") == 0 ) {
                    command c ;
                    strcpy(comm[pos].value,"NOT VALUE");
                    pos++;
                    break;
                }
                // Passo all'indice dell'input
                if (++i >= argc) exit(EXIT_FAILURE);

                // Controllo che l'input non superi la lunghezza massima
                if (strlen(argv[i]) > MAX_OPTION_LEN) exit(EXIT_FAILURE);

                if ( strcmp(command_list[j].type, "int") == 0 ) {
                    sprintf( comm[pos].value,"%d",atoi(argv[i]));
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


