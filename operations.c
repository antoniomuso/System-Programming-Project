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

#include <sys/file.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>


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
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
#endif
};

#ifdef __unix__
char **  build_arguments(char * args) {
    char * pointer = args;
    char * mem_point = NULL;
    int pos = 0;
    int max_len = 5;
    char ** out_arr = calloc(max_len, sizeof(char*));

    char * token = NULL;
    while ((token = strtok_r(pointer, " ", &mem_point)) != NULL) {
        //printf("%s", token);
        pointer = NULL;
        if (pos >= max_len) {
            max_len += 5;
            out_arr = realloc(out_arr, max_len * sizeof(char*));
        }
        char * arg = malloc(strlen(token)+1);
        strcpy(arg, token);
        out_arr[pos] = arg;
        pos++;
    }
    if (pos >= max_len) {
        max_len += 1;
        out_arr = realloc(out_arr, max_len * sizeof(char*));
    }
    out_arr[pos] = NULL;
    return out_arr;
}

#endif

void* thread (void *arg) {
    struct data_args * arguments = (struct data_args*)  arg;
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
        bSuccess = ReadFile(pipe_read, buff, BUFSIZE, &dwRead, NULL);

        fflush(stdout);
        if (read+dwRead >= buff_out_s) {
            buff_out_s *= 2;
            buff_out = realloc(buff_out, buff_out_s);
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

    int fd[2];
    pipe(fd);
    int pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Error during fork of commands");
        arguments->error_out = 1;
        pthread_mutex_lock(&arguments->mutex);
        arguments->done = 1;
        if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "Error during signal of cond variable");
        pthread_mutex_unlock(&arguments->mutex);
        return NULL;
    }  else if (pid == 0) {
        char ** args = build_arguments(arguments->args);
        close(fd[0]);
        dup2(fd[1],1);
        dup2(fd[1],2);
        execvp(arguments->command, args);

        exit(127);
    }

    close(fd[1]);
    int status = 0;
    if (waitpid(pid,&status,0) == -1 ) {
        fprintf(stderr, "Error in waitpid");
        arguments->error_out = 1;
        close(fd[0]);
        pthread_mutex_lock(&arguments->mutex);
        arguments->done = 1;
        if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "Error during signal of cond variable");
        pthread_mutex_unlock(&arguments->mutex);

        return NULL;
    }

    char buff[BUFSIZE];
    int buff_out_s = BUFSIZE;
    char *buff_out = malloc(BUFSIZE);
    int all_read_data = 0;

    fcntl(fd[0], F_SETFL, O_NONBLOCK);

    for (;;) {
        int n_read = read(fd[0], buff, BUFSIZE);

        if (n_read == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
            break;
        }

        if (all_read_data+n_read >= buff_out_s) {
            buff_out_s *= 2;
            buff_out = realloc(buff_out, buff_out_s);

        }

        memcpy(buff_out+all_read_data, buff, n_read);
        all_read_data += n_read;

        if(n_read == 0 || n_read < BUFSIZE) break;

    }

    arguments->error_out = 0;
    arguments->out_size = all_read_data;
    arguments->out = buff_out;
    pthread_mutex_lock(&arguments->mutex);
    arguments->done = 1;
    if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "Error during signal of cond variable");
    pthread_mutex_unlock(&arguments->mutex);
    close(fd[0]);

#endif
    return NULL;
}

int windows_thread (void *arg) {
    thread(arg);
    return 0;
}

int execCommand(int socket, const char * command, const char * args) {
    char * cpy_command = malloc(strlen(command) + 1);
    strcpy(cpy_command, command);

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

    struct data_args * data_arguments = malloc(sizeof(struct data_args));

    data_arguments->fd = socket;
    data_arguments->command = cpy_command;
    data_arguments->args = cpy_args;


#ifdef __unix__

    if (pthread_cond_init(&(data_arguments->cond_var),NULL) != 0) {
        fprintf(stderr,"pthread init failed\n");
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }

    if (pthread_mutex_init(&(data_arguments->mutex), NULL) != 0) {
        fprintf(stderr,"pthread init failed\n");
        free(cpy_command);
        free(cpy_args);
        free(data_arguments);
        return 1;
    }
    //pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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

    pthread_mutex_lock(&(data_arguments->mutex));
    data_arguments->done = 0;
    while (data_arguments->done == 0) {
        pthread_cond_wait(&(data_arguments->cond_var), &(data_arguments->mutex));
    }
    pthread_mutex_unlock(&(data_arguments->mutex));



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

    char * response = create_http_response(200, data_arguments->out_size, "text/html; charset=utf-8", NULL, NULL);

    send(socket, response, strlen(response), 0);
    send(socket, data_arguments->out, data_arguments->out_size, 0);

    free(cpy_command);
    free(cpy_args);
    free(data_arguments);

    return 0;
}

int is_dir(char * url) {
    printf("path %s\n", url);
#ifdef __unix__
    struct stat st;
    stat(url, &st);
    if (S_IFDIR & st.st_mode) {
        return 1;
    }
#elif _WIN32
    DWORD attr = GetFileAttributes((LPCTSTR) url );
    printf("%d\n", attr);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        printf("No dir %d\n", GetLastError());
        LPSTR messageBuffer = NULL;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        printf("%s\n", messageBuffer);
    }
    if (attr != INVALID_FILE_ATTRIBUTES &&
         (attr & FILE_ATTRIBUTE_DIRECTORY)) return 1;
#endif
    return 0;
}



void send_file (int socket, char * url) {

    if (is_dir(url+1) == 1) {
        // send dir content
        printf("path is a directory\n");
        return;
    }

    FILE * pfile;

    pfile = fopen((url+1),"r");
    if (pfile == NULL) {
        //Send a error response
        char * http_h = create_http_response(204,0,NULL, NULL, NULL);
        send(socket,http_h,strlen(http_h),0);
        free(http_h);
        return;
    }

#ifdef __unix__
    int fd = fileno(pfile);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Error during file lock\n");
        // return response with error lock
        fclose(pfile);
    }
#endif

    fseek(pfile, 0, SEEK_END);
    int lengthOfFile = ftell(pfile);
    fseek(pfile, 0, SEEK_SET);


    char * http_h = create_http_response(200,lengthOfFile,"text/html; charset=utf-8", "out.txt",NULL);
    send(socket,http_h, strlen(http_h), 0);
    char * buff = malloc(BUFSIZE);
    int read = 0;
    int remaining_to_read = lengthOfFile;
    printf("send header\n");
    fflush(stdout);


    for(;;) {

        read = fread(buff,1,BUFSIZE,pfile);
        printf("read: %d\n", read);
        fflush(stdout);
        send(socket,buff,read,0);

        if (read == remaining_to_read) {
            break;
        }
        remaining_to_read -= read;
    }
#ifdef __unix__
    while (flock(fd,LOCK_UN) != 0) {
        sleep(100);
    }
    printf("unlock file\n");
#endif
    free(http_h);
    fclose(pfile);
}