//
// Created by anotoniomusolino on 18/06/18.
//

#include "operations.h"
#include "command_parser.h"
#include <stdlib.h>
#include <string.h>
#define TIME_WAIT 6000

#ifdef __unix__

#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

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
    char * args;

    int * error_out;
    char * out;

#ifdef _WIN32
    HANDLE event;
#elif __unix__
    pthread_cond_t cond_var;
#endif
};

void* thread (void *arg) {
    struct data_args * arguments = (struct data_args*)  arg;

#ifdef _WIN32
    STARTUPINFO startup_info = {0};
    PROCESS_INFORMATION proc_info = {0};

    if (!(CreateProcess("windows_process_exe.o", arguments->args, NULL, NULL, FALSE, 0, NULL, NULL, &(startup_info), &(proc_info) ))) {
        fprintf(stderr, "Error occurred while trying to create a process\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    DWORD proc_pid = proc_info.dwProcessId;
#elif __unix__
#endif

}

int windows_thread (void *arg) {
    thread(arg);
    return 0;
}

int execCommand(int socket, const char * command, const char * args) {

    char * cpy_command = malloc(strlen(command) + 1);
    strcpy(cpy_command, command);

    char * cpy_args = malloc(strlen(args)+1);
    strcpy(cpy_args, args);


    struct data_args * data_arguments = malloc(sizeof(struct data_args));

    data_arguments->fd = socket;
    data_arguments->command = cpy_command;
    data_arguments->args = cpy_args;


#ifdef __unix__
    pthread_cond_init(&(data_arguments->cond_var),NULL);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timer;
    timer.tv_sec = TIME_WAIT / 1000;
    timer.tv_nsec = (TIME_WAIT % 1000) * 1000000;

    pthread_t tid;
    if (pthread_create(&tid, NULL, &thread, (void *) data_arguments) != 0) {
        return 1;
    }

    if (pthread_cond_timedwait(&data_arguments->cond_var, &mutex, &timer) != 0) {
        return 1;
    }

#elif _WIN32
    if ((data_arguments->event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        return 1;
    }
    DWORD thr;
    HANDLE thread;
    if ((thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) windows_thread, (LPVOID) data_arguments, 0, &thr)) == NULL) {
        return 1;
    }
    DWORD out = WaitForSingleObject(data_arguments->event, TIME_WAIT);

    if (out == WAIT_TIMEOUT || out == WAIT_FAILED || out == WAIT_ABANDONED ) {
        return 1;
    }
#endif


    return 0;
}