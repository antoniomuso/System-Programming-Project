//
// Created by anotoniomusolino on 31/05/18.
//
#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
 *
 * @param command Command String
 * @param options Commands data struct
 * @return NULL if command isn't contained inside options,
 *         Pointer to a string that it contain the command value.
 */
char* get_command_value (char command[], options options) {
    for (int i = 0; i < options.comm_len; i++) {
        if (strcmp(options.commands[i].name, command) == 0) {
            return options.commands[i].value;
        }
    }
    return NULL;
}

void free_options (options opt) {
    free(opt.commands);
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
    char *ptr = strchr(string, delimiter);

    int len = strlen(string);

    if (ptr) {
        //int index = ptr - string;
        command comm;

        *(ptr) = '\0';
        if (strlen(string) > MAX_OPTION_LEN || strlen(ptr) > MAX_OPTION_LEN) {
            fprintf(stderr, "Command %s exceeds the maximum command length\n", string);
            exit(EXIT_FAILURE);
        }

        strcpy(comm.name,string);
        strcpy(comm.value,ptr+1);


        return comm;
    }

    fprintf(stderr, "Extraction failed.");
    exit(EXIT_FAILURE);
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

    char *buff = malloc(fsize + 2);

    if(fread(buff, fsize, 1, fname) != 1) {
        fprintf(stderr, "Unable to read from %s\n", name);
        exit(EXIT_FAILURE);
    }
    fclose(fname);

    buff[fsize+1] = '\0';

    char *lines = strtok(buff, "\n");

    command* comm = calloc(sizeof(command), arc_len);

    for (int k = 0; k < arc_len; k++) {
        command cmd = extract_command(lines);

        for (int j = 0; j < arc_len; j++) {
            if (strcmp(cmd_arc[j].name, cmd.name) == 0) {
                strcpy(comm[k].name, cmd.name);

                if (strcmp(cmd_arc[j].type, "null") == 0) {
                    strcpy(comm[k].value, "NOT VALUE");
                    break;
                }

                if (strcmp(cmd_arc[j].type, "int") == 0) {
                    sprintf(comm[k].value, "%d", atoi(cmd.value));
                    break;
                }

                if (strcmp(cmd_arc[j].type, "float") == 0) {
                    sprintf(comm[k].value, "%f", atof(cmd.value));
                    break;
                }

                if (strcmp(cmd_arc[j].type, "str") == 0) {
                    strcpy(comm[k].value, cmd.value);
                    break;
                }

            }
        }
        if ((lines = strtok(NULL, "\n")) == NULL) {
            break;
        }
    }
    free(buff);

    options ret;
    ret.comm_len = arc_len;
    ret.commands = comm;

    return ret;
}

http_header http_attribute_parser (http_header http_h, char* data_attr) {
    char attr_separator[] = ":";
    char line_end[] = "\r\n";
    char * in_pointer = NULL;

    char * token = NULL;
    char * sub_token = NULL;

    while ((token = strtok_r(NULL, line_end, &data_attr )) != NULL) {

        in_pointer = NULL;
        sub_token = strtok_r(token, attr_separator, &in_pointer);

        if (strcmp(sub_token,"Authorization") == 0 && in_pointer != NULL) {
            http_h.authorization = in_pointer;
        }

        if (strcmp(sub_token,"User-Agent") == 0 && in_pointer != NULL) {
            http_h.user_agent= in_pointer;
        }

        if (strcmp(sub_token,"Content-Length") == 0 && in_pointer != NULL) {
            http_h.content_length =  atoi(in_pointer);
        }
    }

    return http_h;
}

http_header parse_http_header_request (const char* data, int data_len) {
    char line_end[] = "\r\n";
    char header_end[] = "\r\n\r\n";
    char element_separator[] = " ";

    http_header http_h ;
    // Copy of date
    char * data_copy = malloc(data_len);
    http_h.pointer_to_free = data_copy;
    strcpy(data_copy, data);

    char * token;
    char * sub_token;

    char * in_pointer = NULL;
    char * ext_pointer = NULL;
    // Take first line
    token = strtok_r(data_copy, line_end, &ext_pointer);

    sub_token = strtok_r(token, element_separator, &in_pointer);

    if (strcmp(sub_token,"GET") != 0 && strcmp(sub_token,"PUT") != 0 ) {
        http_h.is_request = -1;
        return http_h;
    }
    http_h.type_req = sub_token;

    sub_token = strtok_r(data_copy, element_separator, &in_pointer);
    http_h.url = sub_token;
    http_h.protocol_type = token;

    http_h = http_attribute_parser(http_h, ext_pointer);
    http_h.is_request = 1;
    return http_h;
}

void free_http_header(http_header http_h) {
    free(http_h.pointer_to_free);
}