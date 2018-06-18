//
// Created by anotoniomusolino on 18/06/18.
//

#include "operations.h"
#include "command_parser.h"
#include <stdlib.h>
#include <string.h>

#ifdef __unix__

#include <pthread.h>
#include <sys/socket.h>

#elif _WIN32

# undef  _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_WINXP
# undef  WINVER
# define WINVER       _WIN32_WINNT_WINXP

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws_32.lib")
#include <windows.h>

#endif

struct data_args {
    int fd;
    char * command;
    char ** args;
    int argc;
    char * out;
#ifdef _WIN32
    HANDLE event;
#endif
};

void* thread (void *arg) {
    struct data_args * arguments = (struct data_args*)  arg;


}

int windows_thread (void *arg) {
    thread(arg);
    return 0;
}

int execCommand(int socket, const char * command, const char ** args, const int argc ) {

    char * cpy_command = malloc(strlen(command) + 1);
    strcpy(cpy_command, command);

    char ** cpy_args = calloc(argc ,sizeof(char*));

    for (int i = 0; i < argc; i++) {
        cpy_args[i] = malloc( strlen(args[i]) + 1);
        strcpy(cpy_args[i], args[i]);
    }

    struct data_args * data_arguments = malloc(sizeof(struct data_args));

    data_arguments->fd = socket;
    data_arguments->command = cpy_command;
    data_arguments->args = cpy_args;
    data_arguments->argc = argc;


#ifdef __unix__
    pthread_t tid;
    if (pthread_create(&tid, NULL, &thread, (void *) data_arguments) != 0) {
        return 1;
    }
#elif _WIN32
    DWORD thr;
    if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) windows_thread, (LPVOID) data_arguments, 0, &thr) == NULL) {
        return 1;
    }
#endif

    return 0;
}