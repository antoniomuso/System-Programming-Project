//
// Created by anotoniomusolino on 31/05/18.
//
#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif


#ifdef _WIN32
#include <windows.h>

#elif __unix__
#include <linux/limits.h>

#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif


//#ifdef _WIN32
#ifndef strtok_r
/*-
 * Copyright (c) 1998 Softweyr LLC.  All rights reserved.
 *
 * strtok_r, from Berkeley strtok
 * Oct 13, 1998 by Wes Peters <wes@softweyr.com>
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOFTWEYR LLC, THE REGENTS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SOFTWEYR LLC, THE
 * REGENTS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _POSIX_SOURCE 1
/*
 * strtok_r documentation:
 * http://pubs.opengroup.org/onlinepubs/009695399/functions/strtok.html
 *
 * Implementation:
 * http://svnweb.freebsd.org/base/head/lib/libc/string/strtok.c?view=co
 *
 * strtok_r cannot completely be implemented by strtok because of the internal state.
 * It breaks when used on 2 strings where they are scanned A, B then A, B.
 * Thread-safety is not the issue.
 *
 * Sample strtok implemenatation, note the internal state:
 * char *strtok(char *s, const char *d) { static char *t; return strtok_r(s,d,t); }
 *
 */
char *
strtok_r(char * __restrict s, const char * __restrict delim, char ** __restrict last)
{
	char *spanp, *tok;
	int c, sc;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
#endif

#define MAX_BUFF_LEN 8000

/**
 *
 * @param command Command String
 * @param opt Commands data struct
 * @return NULL if command isn't contained inside options,
 *         Pointer to a string that it contain the command value.
 */
char* get_command_value (char command[], options opt) {
    if (opt.commands == NULL) return NULL;

    for (int i = 0; i < opt.comm_len; i++) {
        if (strcmp(opt.commands[i].name, command) == 0) {
            return opt.commands[i].value;
        }
    }
    return NULL;
}

int contains (const char name[MAX_OPTION_LEN],const char value[MAX_OPTION_LEN], options opt) {
    for (int i = 0; i < opt.comm_len; i++ ) {
        if (strcmp(opt.commands[i].name, name) == 0
            && strcmp(opt.commands[i].value , value) == 0) {
            return 1;
        }
    }
    return 0;
}

void free_options (options opt) {
    free(opt.commands);
}

/**
 *
 * @param src on form: Basic am9lYjp4eDEyMw==
 * @param res Struct: Username, Password
 */
authorization parse_authorization (const char * src) {
    char buff[strlen(src)];
    strcpy(buff,src);

    authorization auth;
    auth.free_pointer = NULL;

    char * pointer;
    char * token = NULL;
    if ( ((token = strtok_r(buff, " ", &pointer)) == NULL)
            || ((token = strtok_r(NULL, " ", &pointer)) == NULL)) {
        return auth;
    }


    unsigned char * decode = b64_decode(token, strlen(token));

    if (decode == NULL) return auth;

    pointer = NULL;

    if ( ((token = strtok_r(decode, ":", &pointer)) == NULL)) {
        free(decode);
        return auth;
    }
    auth.name = token;

    if ( ((token = strtok_r(NULL, ":", &pointer)) == NULL)) {
        free(decode);
        return auth;
    }
    auth.password = token;

    auth.free_pointer = decode;

    return auth;
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

    if (ptr != NULL) {
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

    fprintf(stderr, "Extraction failed.\n");
    //exit(EXIT_FAILURE);
}

options parse_file(char *name, command_arc * cmd_arc, int arc_len) {
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

    if(fread(buff, 1, fsize, fname) < fsize) {
        fprintf(stderr, "Unable to read from %s\n", name);
        exit(EXIT_FAILURE);
    }
    fclose(fname);

    buff[fsize] = '\0';

    char * pointer = NULL;

    int len = 20;

    command* comm = calloc(sizeof(command), len);

    char * p = buff;

    char * lines;

    int k = 0;
    while ((lines = strtok_r(p, "\n", &pointer)) != NULL) {
        fflush(stdout);
        p = NULL;
        command cmd = extract_command(lines);

        if (k >= len) {
            len+= REALLOC_INC_SIZE;
            comm = (command*) realloc(comm, sizeof(command) * len);

            if (comm == NULL) {
                fprintf(stderr,"Realloc Failed\n");
                exit(EXIT_FAILURE);
            }
        }

        if (cmd_arc == NULL){
            strcpy(comm[k].value, cmd.value);
            strcpy(comm[k].name, cmd.name);
            k++;
            continue;
        }

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
        k++;
    }
    free(buff);

    options ret;
    ret.comm_len = k;
    ret.commands = comm;

    return ret;
}



http_header http_attribute_parser (http_header http_h, char* data_attr) {
    char attr_separator[] = ":";
    char line_end[] = "\r\n";
    char * in_pointer = NULL;

    char * token = NULL;
    char * sub_token = NULL;
    char * attr_value = NULL;

    while ((token = strtok_r(NULL, line_end, &data_attr )) != NULL) {

        in_pointer = NULL;

        sub_token = strtok_r(token, attr_separator, &in_pointer);
        attr_value = strtok_r(NULL, attr_separator, &in_pointer);

        if (strcmp(sub_token,"Authorization") == 0 && attr_value != NULL) {
            http_h.attribute.authorization = attr_value;
        }

        if (strcmp(sub_token,"User-Agent") == 0 && attr_value != NULL) {
            http_h.attribute.user_agent = attr_value;
        }

        if (strcmp(sub_token,"Content-Length") == 0 && attr_value != NULL) {
            http_h.attribute.content_length =  atoi(attr_value);
        }

        if (strcmp(sub_token,"Content-Type") == 0 && attr_value != NULL) {
            http_h.attribute.content_type =  attr_value;
        }

        if (strcmp(sub_token,"Connection") == 0 && attr_value != NULL) {
            http_h.attribute.connection =  attr_value;
        }

    }

    return http_h;
}


http_header parse_http_header_response(const char *data, int data_len) {
    char line_end[] = "\r\n";
    char header_end[] = "\r\n\r\n";
    char element_separator[] = " ";

    http_header http_h = {0} ;
    http_h.is_request = 0;

    if (data_len > MAX_BUFF_LEN) {
        http_h.is_request = -1;
        return http_h;
    }
    // Copy of data
    char * data_copy = malloc(data_len+1);
    http_h.pointer_to_free = data_copy;
    memcpy(data_copy, data, data_len);
    data_copy[data_len] = '\0';

    char * token;
    char * sub_token;

    char * in_pointer = NULL;
    char * ext_pointer = NULL;
    // Take first line
    token = strtok_r(data_copy, line_end, &ext_pointer);



    if (token == NULL
                        || ((sub_token = strtok_r(token, element_separator, &in_pointer)) == NULL)
                        || (strcmp("HTTP/1.1", sub_token) != 0 && strcmp("HTTP/1.0", sub_token) != 0) ) {
        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }



    http_h.protocol_type = sub_token;

    if ( ((sub_token = strtok_r(NULL, element_separator, &in_pointer)) == NULL)
                        || (http_h.code_response = atoi(sub_token)) == 0) {
        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }

    http_h = http_attribute_parser(http_h, ext_pointer);
    return http_h;
}


http_header parse_http_header_request (const char* data, int data_len) {
    char line_end[] = "\r\n";
    char header_end[] = "\r\n\r\n";
    char element_separator[] = " ";

    http_header http_h = {0};
    http_h.is_request = 1;

    if (data_len > MAX_BUFF_LEN) {
        http_h.is_request = -1;
        return http_h;
    }

    // Copy of date
    char * data_copy = malloc(data_len+1);
    http_h.pointer_to_free = data_copy;
    memcpy(data_copy, data, data_len);
    data_copy[data_len] = '\0';

    char * token;
    char * sub_token;

    char * in_pointer = NULL;
    char * ext_pointer = NULL;
    // Take first line
    token = strtok_r(data_copy, line_end, &ext_pointer);

    if (token == NULL
                    || ((sub_token = strtok_r(token, element_separator, &in_pointer)) == NULL)
                    || (strcmp(sub_token,"GET") != 0 && strcmp(sub_token,"PUT") != 0) ) {

        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }

    http_h.type_req = sub_token;

    if ((sub_token = strtok_r(NULL, element_separator, &in_pointer)) == NULL) {
        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }

    http_h.url = sub_token;

    if ((sub_token = strtok_r(NULL, element_separator, &in_pointer)) == NULL) {
        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }

    http_h.protocol_type = sub_token;

    if (strcmp("HTTP/1.1", http_h.protocol_type) != 0 && strcmp("HTTP/1.0", http_h.protocol_type) != 0) {
        http_h.is_request = -1;
        free(data_copy);
        return http_h;
    }

    http_h = http_attribute_parser(http_h, ext_pointer);

    return http_h;
}

char *code_to_message(int code) {
    switch (code){
        case 100:
            return "Continue";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 302:
            return "Found";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
    }
}

char *create_http_response(int response_code, unsigned long content_len, char * content_type, char *location) {
    //NB: Still need to handle the translation of content_type
    const unsigned int MAX_CONTENT_LEN = (52 + 20 + MAX_HTTP_FIELD_LEN);

    // Max path len
    if ( (content_type != NULL && strlen(content_type) > PATH_MAX)
         || (location != NULL && strlen(location) > MAX_HTTP_FIELD_LEN)) {
        return NULL;
    }

    char *response = calloc(MAX_CONTENT_LEN + PATH_MAX + 100, 1);

    char *content = NULL;
    if (content_len != -1 && content_type != NULL) {
        content = malloc(MAX_CONTENT_LEN);
        int err = snprintf(content, MAX_CONTENT_LEN, "Accept-Ranges: bytes\r\n"
                                                     "Connection: keep-alive\r\n"
                                        "Content-Length: %ld\r\n"
                                        "Content Type: %s\r\n",
                 content_len, content_type == NULL ? "" : content_type);

        if (err == -1) {
            free(content);
            free(response);
            return NULL;
        }
    }


    char *loc = NULL;
    if (location != NULL) {
        loc = malloc(PATH_MAX + 13);
        snprintf(loc, PATH_MAX + 13, "Location: %s\r\n", location);
    }

    int len = snprintf(response, MAX_CONTENT_LEN + PATH_MAX + 100, "HTTP/1.0 %d %s\r\n"
                                                                  "%s%s"
                                                                  "\r\n"
                                                      , response_code, code_to_message(response_code), content, loc == NULL ? "" : loc);


    if (len == -1) {
        free(content);
        free(response);
        return NULL;
    }



    char *final_response = calloc(strlen(response), 1);
    memcpy(final_response, response, len); //This way, final_response should not be null-terminated.

    free(content);
    free(loc);
    free(response);

    return final_response;
}

void free_http_header(http_header http_h) {
    free(http_h.pointer_to_free);
}

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

struct operation_command parser_operation (char * url) {
    struct operation_command out;
    out.args = NULL;
    out.comm = NULL;
    if (!startsWith("/command/", url)) return out;

}