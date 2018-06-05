//
// Created by anotoniomusolino on 31/05/18.
//
#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LEN(x)  (sizeof(x) / sizeof((x)[0]))

/**
 *
 * @param command Command String
 * @param options Commands data struct
 * @return NULL if command isn't contained inside options,
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
                        fprintf(stderr, "Options realloc failed\n");
                        exit(EXIT_FAILURE);
                    }
                }

                if (strlen(argv[i]) > MAX_OPTION_LEN) {
                    fprintf(stderr, "Command %s exceeds the maximum command length\n", argv[i]);
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
                    fprintf(stderr, "Iput for command %s not passed\n", argv[i-1]);
                    exit(EXIT_FAILURE);
                }

                // Controllo che l'input non superi la lunghezza massima
                if (strlen(argv[i]) > MAX_OPTION_LEN) {
                    fprintf(stderr, "Command input %s exceeds the maximum command length\n", argv[i]);
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

command extract_command(char *string) {
    /**
     * Receives one string whose format is "field=value" and
     * extracts information from it.
     * @return a command struct containing the extracted info.
     */
    char delimiter = '=';
    const char *ptr = strchr(string, delimiter);
    int len = LEN(string);
    if (ptr) {
        int index = ptr - string;
        command comm;
        char cmd[index + 1];
        sprintf(cmd, "%.*s", index, string);
        strcpy(comm.name, cmd);
        char val[len - index - 1];
        sprintf(val, "%.*s", len - index - 1, string + index + 1);
        strcpy(comm.value, val);
        return comm;
    }
    else {
        fprintf(stderr, "Extraction failed.");
        exit(EXIT_FAILURE);
    }
}

options parse_file(char *name, command_arc cmd_arc[], int arc_len) {
    /**
     * Parses a file following the chosen format.
     */
    FILE *fname = fopen(name, "r");
    if (fname == NULL) {
        fprintf(stderr, "Unable to open file %s\n", name);
        exit(EXIT_FAILURE);
    }
    fseek(fname, 0, SEEK_END);
    long fsize = ftell(fname);
    fseek(fname, 0, SEEK_SET);
    char *text = malloc(fsize + 1);
    if(fread(text, fsize, 1, fname) != 1) {
        fprintf(stderr, "Unable to read from %s\n", name);
        exit(EXIT_FAILURE);
    }
    fclose(fname);
    text[fsize] = 0;
    //printf("File content: %s\n", text);

    options ret;

    char *lines = strtok(text, "\n");
    int len = LEN(lines);

    command* comm = calloc(sizeof(command), len);

    for (int k = 0; k < len; k++) {
        printf("LINE (%d): %s\n", k, lines);
        char tmp[LEN(lines)];
        strcpy(tmp, lines);
        command cmd = extract_command(tmp);
        printf("cmd out: %s %s\n", cmd.name, cmd.value);
        for (int j = 0; j < arc_len; j++) {
            if (strcmp(cmd_arc[j].name, cmd.name) == 0) {
                printf("%s == %s\n", cmd_arc[j].name, cmd.name);


                if (strcmp(cmd_arc[j].type, "null") == 0) {
                    strcpy(comm[k].value, "NOT VALUE");
                    break;
                }

                if (strlen(cmd.value) > MAX_OPTION_LEN) {
                    fprintf(stderr, "Command input %s exceeds the maximum command length\n", cmd.value);
                    exit(EXIT_FAILURE);
                }

                //Problem (SEGMENTATION FAULT) lies from here
//                strcpy(comm[k].name, cmd.name); //Problem here
//
                if (strcmp(cmd_arc[j].type, "int") == 0) {
                    printf("int\n");
                    //sprintf(comm[k].value, "%d", atoi(cmd.value));
                    break;
                }

                if (strcmp(cmd_arc[j].type, "float") == 0) {
                    printf("float\n");
                    //sprintf(comm[k].value, "%f", atof(cmd.value));
                    break;
                }

                if (strcmp(cmd_arc[j].type, "str") == 0) {
                    printf("str\n");
                    //strcpy(comm[k].value, cmd.value);
                    break;
                }
                //to here
            }
        }
        printf("Over\n\n");
        lines = strtok(NULL, "\n");
        if (lines == NULL) {
            break;
        }
    }


    ret.comm_len = len;
    ret.commands = comm;
    return ret;
}

http_header parse_http_request (char* data, int data_len) {
    const char line_end[] = "\r\n";
    const char header_end[] = "\r\n\r\n";



}





