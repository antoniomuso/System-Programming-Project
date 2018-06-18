//
// Created by anotoniomusolino on 18/06/18.
//

#include "operations.h"
#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define TIME_WAIT 6000
#define BUFSIZE 4096

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

    int error_out;
    int out_size;
    char * out;

#ifdef _WIN32
    HANDLE event;
#elif __unix__
    pthread_cond_t cond_var;
#endif
};

void* thread (void *arg) {
    struct data_args * arguments = (struct data_args*)  arg;

    printf("Entering Thread\n");
    fflush(stdout);
#ifdef _WIN32

    HANDLE pipe_read = NULL;
    HANDLE child_write = NULL;

    SECURITY_ATTRIBUTES sattr;

    sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sattr.bInheritHandle = TRUE;
    sattr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&pipe_read, &child_write, &sattr , 0)) {
        fprintf(stderr, "Error occurred while creating pipe\n");
        fflush(stderr);
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed");
        return NULL;
    }
    if (! SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0) ) {
        fprintf(stderr, "Error occurred with pipe\n");
        fflush(stderr);
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed");
        return NULL;
    }

    STARTUPINFO startup_info;
    PROCESS_INFORMATION proc_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));
    ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));

    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdOutput = child_write;
    startup_info.hStdError = child_write;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    if (!(CreateProcess(arguments->command, arguments->args, NULL, NULL, TRUE, 0, NULL, NULL, &(startup_info), &(proc_info) ))) {
        fprintf(stderr, "Error occurred while trying to create a process\n");
        fflush(stderr);
        arguments->error_out = 1;
        CloseHandle(pipe_read);
        CloseHandle(child_write);
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed");
        return NULL;
    }



    DWORD proc_pid = proc_info.dwProcessId;
    HANDLE proc_handle = proc_info.hProcess;

    DWORD out = WaitForSingleObject(proc_handle, TIME_WAIT);
    if (out == WAIT_TIMEOUT || out == WAIT_FAILED || out == WAIT_ABANDONED ) {
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed");
        CloseHandle(pipe_read);
        CloseHandle(child_write);
        return NULL;
    }

    printf("Process terminated\n");
    fflush(stdout);

    DWORD dwRead;
    CHAR buff[BUFSIZE];
    int buff_out_s = BUFSIZE;
    CHAR *buff_out = malloc(BUFSIZE);
    BOOL bSuccess = FALSE;
    int read = 0;

    for (;;) {

        printf("Starting to read\n");
        fflush(stdout);
        fflush(stdout);
        bSuccess = ReadFile(pipe_read, buff, BUFSIZE, &dwRead, NULL);

        fflush(stdout);
        if (read+dwRead >= buff_out_s) {
            buff_out = realloc(buff_out, buff_out_s*2);
            buff_out_s *= 2;
        }

        memcpy(buff_out+read, buff, dwRead);
        read += dwRead;
        if( ! bSuccess || dwRead == 0 || dwRead < BUFSIZE) break;
    }


    arguments->error_out = 0;
    arguments->out_size = read;
    arguments->out = buff_out;

    if (SetEvent(arguments->event) == FALSE) {
        fprintf(stderr, "SetEvent Failed");
        arguments->error_out = 1;
        CloseHandle(pipe_read);
        CloseHandle(child_write);
        free(buff_out);
        return NULL;
    }
    CloseHandle(pipe_read);
    CloseHandle(child_write);
#elif __unix__
#endif
    return NULL;
}

int windows_thread (void *arg) {
    thread(arg);
    return 0;
}

int execCommand(int socket, const char * command, const char * args) {

    printf("Entering execCommand\n");
    fflush(stdout);
    char * cpy_command = malloc(strlen(command) + 1);
    strcpy(cpy_command, command);

    printf("strcpy cpyargs1\n");
    fflush(stdout);

    char *cpy_args = NULL;
    if (args != NULL) {
        cpy_args = malloc(strlen(args) + 1);
        strcpy(cpy_args, args);
        if (cpy_args == NULL) {
            printf("cpyargs is null\n");
            fflush(stdout);
            return 1;
        }
    }




    printf("strcpy cpyargs2\n");
    fflush(stdout);


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
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }

    if (pthread_cond_timedwait(&data_arguments->cond_var, &mutex, &timer) != 0) {
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }

#elif _WIN32
    if ((data_arguments->event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }
    DWORD thr;
    HANDLE thread;
    if ((thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) windows_thread, (LPVOID) data_arguments, 0, &thr)) == NULL) {
        fprintf(stderr, "Error Occurred while trying to create thread\n");
        fflush(stderr);
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }

    printf("Entering WaitForThreadEvent\n");
    fflush(stdout);

    DWORD out = WaitForSingleObject(data_arguments->event, TIME_WAIT);

    if (out == WAIT_TIMEOUT || out == WAIT_FAILED || out == WAIT_ABANDONED ) {
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }
#endif
    if (data_arguments->error_out == 1) {
        fprintf(stderr, "Command Execution Failed\n");
        fflush(stderr);
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }

    //data_arguments->out[data_arguments->out_size] = '\0';
    printf("creating respn\n");
    fflush(stdout);



    char * response = create_http_response(200, data_arguments->out_size, "text/html; charset=utf-8", NULL);

    char output[strlen(response)+data_arguments->out_size];
    memcpy(output, response, strlen(response));
    memcpy(output+strlen(response), data_arguments->out, data_arguments->out_size);

    printf("%s\n", output);
    fflush(stdout);
    send(socket, output, strlen(response)+data_arguments->out_size, 0);
    //send(socket, data_arguments->out, data_arguments->out_size, 0);


    free(cpy_command);
    free(cpy_args);
    free(data_arguments);

    return 0;
}