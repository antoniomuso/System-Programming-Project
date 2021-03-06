#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#define BUFSIZE 4096

#ifdef __unix__

#include <dirent.h>
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
#include <sys/mman.h>


#elif _WIN32

# undef  _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_WINXP
# undef  WINVER
# define WINVER       _WIN32_WINNT_WINXP

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws_32.lib")
#include <windows.h>
#include <io.h>

#endif

#ifndef rand_r
/* Reentrant random function from POSIX.1c.
   Copyright (C) 1996-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits.  */
int
rand_r (unsigned int *seed)
{
    unsigned int next = *seed;
    int result;
    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;
    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;
    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;
    *seed = next;
    return result;
}
#endif



void m_sleep (unsigned int time) {
#ifdef __unix__
    sleep(time);
#elif _WIN32
    Sleep(time * 1000);
#endif

}

int Send(int socket, const void * buff, int size, int flag) {
    int res = send(socket,buff ,size ,0);
    if (res == -1) {
        fprintf(stderr, "An error occurred while trying to use the send function.\n");
    }
    return res;
}


static const char LOGFILE[] = "log.txt";

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
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
#endif
};


int lock_file (FILE * file, long len, int blocking) {
#ifdef __unix__
    int fd = fileno(file);
    int flag = blocking == 1 ? LOCK_EX : LOCK_EX | LOCK_NB;
    if (flock(fd,flag) != 0) {
        return 1;
    }
#elif _WIN32
    HANDLE file_h = (HANDLE)_get_osfhandle(_fileno( file ));
    OVERLAPPED sOverlapped = {0} ;
    DWORD flag = blocking == 1 ? LOCKFILE_EXCLUSIVE_LOCK : LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
    if (LockFileEx(file_h, flag , 0,(DWORD) len, 0, &sOverlapped) == FALSE) {
        fprintf(stderr, "Couldn't acquire lock on file.\n");
        return 1;
    }
#endif
    return 0;
}

int unlock_file (FILE * file, long len) {
#ifdef __unix__
    int fd = fileno(file);
    if (flock(fd,LOCK_UN) != 0) {
        return 1;
    }
#elif _WIN32
    HANDLE file_h = (HANDLE)_get_osfhandle(_fileno( file ));
    OVERLAPPED sOverlapped = {0} ;
    if (UnlockFileEx(file_h, 0,(DWORD) len, 0, &sOverlapped) == FALSE) {
        fprintf(stderr, "Couldn't release lock from file.\n");
        return 1;
    }
#endif
    return 0;
}

int log_write(char * cli_addr, char * user_id, char * username, char * request, char * url,
              char * protocol_type, int return_code, int bytes_sent) {

    FILE *logfile = fopen(LOGFILE, "a");
    if (logfile == NULL) {
        fprintf(stderr, "Couldn't open logfile.\n");
        return 1;
    }

#ifdef __unix__
    int fd = fileno(logfile);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Couldn't acquire lock on file\n");
        fclose(logfile);
        return 1;
    }
#endif
    int timestr_len = 27;
    char timestamp_str[timestr_len];

    time_t timestamp = time(NULL);
    struct tm *lat = localtime(&timestamp);

    strftime(timestamp_str, timestr_len,  "%d/%b/%Y:%H:%M:%S", lat);

    int buff_len = strlen(cli_addr)
                   + (user_id == NULL ? 1 : strlen(user_id))
                   + (username == NULL ? 1 : strlen(username)) +
                   + strlen(timestamp_str) + 20 + strlen(request) + strlen(url) +  strlen(protocol_type) + 3 + 10 + 10;
    char log_string[buff_len + strlen("\n")];

    snprintf(log_string, buff_len+strlen("\n"), "%s %s %s [%s %li] \"%s %s %s\" %d %d\n", cli_addr,
             user_id == NULL ? "-" : user_id,
             username == NULL ? "-" : username,
             timestamp_str, timezone, request, url, protocol_type, return_code, bytes_sent);
    //Nota timezone è -3600 (= 1 ora) perché tiene in considerazione l'ora legale (che è UTC+1)

    if (fwrite(log_string, 1, strlen(log_string), logfile) == 0) {
        fprintf(stderr, "An error occurred while trying to write to logfile");
        return 1;
    }

#ifdef __unix__
    while (flock(fd,LOCK_UN) != 0) {
        sleep(1);
    }
#endif

    fclose(logfile);
    return 0;
}

int http_log (http_header h_request, char * h_response, char * client_address, int no_name) {
    http_header h_resp = parse_http_header_response(h_response,strlen(h_response));
    if (h_resp.is_request == -1) {
        fprintf(stderr, "An error occurred while trying to parse the HTTP response.\n");
        return 1;
    }
    int err = 0;
    if (!no_name) {
        if (h_request.attribute.authorization == NULL) return 1;
        authorization auth = parse_authorization(h_request.attribute.authorization);

        if (auth.free_pointer == NULL) {
            free_http_header(h_resp);
            return 1;
        }
        err = log_write(client_address, NULL, auth.name, h_request.type_req, h_request.url,
                  h_request.protocol_type, h_resp.code_response, h_resp.attribute.content_length);
        free(auth.free_pointer);
        free_http_header(h_resp);
        return err;
    }

    err = log_write(client_address, NULL, NULL, h_request.type_req, h_request.url,
              h_request.protocol_type, h_resp.code_response, h_resp.attribute.content_length);

    free_http_header(h_resp);
    return err;
}



#ifdef __unix__
char **  build_arguments(char * args) {
    char * pointer = args;
    char * mem_point = NULL;
    int pos = 0;
    int max_len = 5;
    char ** out_arr = calloc(max_len, sizeof(char*));
    if (out_arr == NULL) {
        return NULL;
    }

    char * token = NULL;
    while ((token = strtok_r(pointer, " ", &mem_point)) != NULL) {
        //printf("%s", token);
        pointer = NULL;
        if (pos >= max_len) {
            max_len += 5;
            out_arr = realloc(out_arr, max_len * sizeof(char*));
        }
        char * arg = malloc(strlen(token)+1);
        if (arg == NULL) {
            free(out_arr);
            return NULL;
        }
        strcpy(arg, token);
        out_arr[pos] = arg;
        pos++;
    }
    if (pos >= max_len) {
        max_len += 2;
        char ** real = NULL;
        real = realloc(out_arr, max_len * sizeof(char*));
        if (real == NULL) {
            free(out_arr);
            return NULL;
        }
        out_arr = real;

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
        fprintf(stderr, "An error occurred while trying to create the Pipe.\n");
        fflush(stderr);
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
        return NULL;
    }
    if (! SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0) ) {
        fprintf(stderr, "Pipe error.\n");
        fflush(stderr);
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
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
        fprintf(stderr, "An error occurred while trying to create the process.\n");
        fflush(stderr);
        arguments->error_out = 1;
        CloseHandle(pipe_read);
        CloseHandle(child_write);
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
        return NULL;
    }

    DWORD proc_pid = proc_info.dwProcessId;
    HANDLE proc_handle = proc_info.hProcess;

    printf("Process terminated.\n");
    fflush(stdout);

    DWORD dwRead;
    CHAR buff[BUFSIZE];
    int buff_out_s = BUFSIZE;
    CHAR *buff_out = malloc(buff_out_s);

    if (buff_out == NULL) {
        arguments->error_out = 1;
        if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
        CloseHandle(pipe_read);
        CloseHandle(child_write);
        return NULL;
    }

    BOOL bSuccess = FALSE;
    int read = 0;

    for (;;) {

        printf("Starting to read\n");
        fflush(stdout);

        DWORD out = WaitForSingleObject(proc_handle, 50);
        if (out == WAIT_FAILED || out == WAIT_ABANDONED ) {
            arguments->error_out = 1;
            free(buff_out);
            if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
            CloseHandle(pipe_read);
            CloseHandle(child_write);
            return NULL;
        }
        DWORD peek_read = 0;
        if(PeekNamedPipe(pipe_read, NULL, 0, NULL, &peek_read, NULL) == FALSE) {
            arguments->error_out = 1;
            free(buff_out);
            if(SetEvent(arguments->event) == FALSE)
            fprintf(stderr, "SetEvent Failed.\n");
            CloseHandle(pipe_read);
            CloseHandle(child_write);
            return NULL;
        }

        if (peek_read == 0 && out != WAIT_OBJECT_0 ) continue;

        if (peek_read == 0) break;

        bSuccess = ReadFile(pipe_read, buff, BUFSIZE, &dwRead, NULL);

        if (read+dwRead >= buff_out_s) {
            buff_out_s *= 2;
            char * real = NULL;
            real = realloc(buff_out, buff_out_s);
            if (real == NULL) {
                arguments->error_out = 1;
                if(SetEvent(arguments->event) == FALSE) fprintf(stderr, "SetEvent Failed.\n");
                CloseHandle(pipe_read);
                CloseHandle(child_write);
                free(buff_out);
                return NULL;
            }
            buff_out = real;
        }

        memcpy(buff_out+read, buff, dwRead);
        read += dwRead;
        if( ! bSuccess || dwRead == 0 || dwRead < BUFSIZE) break;
    }

    arguments->error_out = 0;
    arguments->out_size = read;
    arguments->out = buff_out;

    if (SetEvent(arguments->event) == FALSE) {
        fprintf(stderr, "SetEvent Failed.\n");
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
    if (pipe(fd) == -1) {
        arguments->error_out = 1;
        pthread_mutex_lock(&arguments->mutex);
        if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
        pthread_mutex_unlock(&arguments->mutex);
        return NULL;
    }
    int pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Command fork failed.\n");
        arguments->error_out = 1;
        pthread_mutex_lock(&arguments->mutex);
        if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
        pthread_mutex_unlock(&arguments->mutex);
        return NULL;
    }  else if (pid == 0) {
        char ** args = build_arguments(arguments->args);
        if (args == NULL) {
            fprintf(stderr,"build_arguments failed.\n");
            exit(127);
        }
        close(fd[0]);
        dup2(fd[1],1);
        dup2(fd[1],2);
        execvp(arguments->command, args);

        exit(127);
    }

    close(fd[1]);

    char buff[BUFSIZE];
    int buff_out_s = BUFSIZE;
    char *buff_out = malloc(buff_out_s);
    if (buff_out == NULL) {
        fprintf(stderr,"malloc error.\n");
        arguments->error_out = 1;
        close(fd[0]);
        pthread_mutex_lock(&arguments->mutex);
        if (pthread_cond_signal(&arguments->cond_var) != 0) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
        pthread_mutex_unlock(&arguments->mutex);
        return NULL;
    }
    int all_read_data = 0;

    fcntl(fd[0], F_SETFL, O_NONBLOCK);
    int status = 0;

    for (;;) {

        int ex = waitpid(pid,&status,WNOHANG);

        if (ex < 0  && errno != ECHILD) {
            fprintf(stderr, "Error in waitpid: %s\n", strerror(errno));
            arguments->error_out = 1;
            close(fd[0]);
            free(buff_out);
            pthread_mutex_lock(&arguments->mutex);
            if (pthread_cond_signal(&arguments->cond_var) != 0) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
            pthread_mutex_unlock(&arguments->mutex);
            return NULL;
        }

        int n_read = read(fd[0], buff, BUFSIZE);

        // se non ho letto nnt ma il processo non ha terminato esco.
        if (n_read == -1 && (errno == EWOULDBLOCK || errno == EAGAIN) && ex == 0) {
            continue;
        }

        if (n_read == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) break;

        if (n_read == -1 ) {
            printf("%s", strerror(errno));
            fflush(stdout);
            free(buff_out);
            arguments->error_out = 1;
            pthread_mutex_lock(&arguments->mutex);
            if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
            pthread_mutex_unlock(&arguments->mutex);
            close(fd[0]);
            return NULL;
        }

        if (all_read_data+n_read >= buff_out_s) {
            buff_out_s *= 2;
            char * real = NULL;
            real = realloc(buff_out, buff_out_s);
            if (real == NULL) {
                fprintf(stderr,"realloc error.\n");
                arguments->error_out = 1;
                close(fd[0]);
                free(buff_out);
                pthread_mutex_lock(&arguments->mutex);
                if (pthread_cond_signal(&arguments->cond_var) != 0) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
                pthread_mutex_unlock(&arguments->mutex);
                return NULL;
            }
            buff_out = real;
        }

        memcpy(buff_out+all_read_data, buff, n_read);
        all_read_data += n_read;

        if((n_read == 0 || n_read < BUFSIZE) && ex != 0) break;
    }

    int exit = WEXITSTATUS(status);

    if (exit == 127) {
        fprintf(stderr, "Command not found %s\n", strerror(errno));
        arguments->error_out = 1;
        close(fd[0]);
        pthread_mutex_lock(&arguments->mutex);
        if (pthread_cond_signal(&arguments->cond_var) != 0) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
        pthread_mutex_unlock(&arguments->mutex);
        return NULL;
    }

    arguments->error_out = 0;
    arguments->out_size = all_read_data;
    arguments->out = buff_out;
    pthread_mutex_lock(&arguments->mutex);
    if (pthread_cond_signal(&arguments->cond_var)) fprintf(stderr, "An error occurred while trying to signal the condion variable.\n");
    pthread_mutex_unlock(&arguments->mutex);
    close(fd[0]);

#endif
    return NULL;
}

int windows_thread (void *arg) {
    thread(arg);
    return 0;
}

int exec_command(int socket, const char * command, const char * args, http_header http_h, char * address) {
    char * cpy_command = malloc(strlen(command) + 1);
    if (cpy_command == NULL) {
        return 1;
    }
    strcpy(cpy_command, command);

    char *cpy_args = NULL;
    if (args != NULL) {
        cpy_args = malloc(strlen(args) + 1);
        if(cpy_args == NULL) {
            free(cpy_command);
            return 1;
        }
        strcpy(cpy_args, args);
    }

    struct data_args data_arguments ;

    data_arguments.fd = socket;
    data_arguments.command = cpy_command;
    data_arguments.args = cpy_args;

#ifdef __unix__

    if (pthread_cond_init(&(data_arguments.cond_var),NULL) != 0) {
        fprintf(stderr,"pthread_cond_init failed.\n");
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    if (pthread_mutex_init(&(data_arguments.mutex), NULL) != 0) {
        fprintf(stderr,"pthread_mutex_init failed.\n");
        free(cpy_command);
        free(cpy_args);
        return 1;
    }
    pthread_t tid;

    if (pthread_create(&tid, NULL, &thread, (void *) (&data_arguments)) != 0) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    if (pthread_mutex_lock(&(data_arguments.mutex)) != 0) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }


    if (pthread_cond_wait(&(data_arguments.cond_var), &(data_arguments.mutex)) != 0) {
        fprintf(stderr,"Error pthread_cond_wait\n");
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    // disalloc thread memory
    if (pthread_join(tid,NULL) != 0) {
        fprintf(stderr,"Error join of thread\n");
        free(cpy_command);
        free(cpy_args);
        return 1;
    }


    if (pthread_mutex_unlock(&(data_arguments.mutex)) != 0) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    // Destroy mutex and condition variable
    pthread_cond_destroy(&(data_arguments.cond_var));
    pthread_mutex_destroy(&(data_arguments.mutex));


#elif _WIN32
    if ((data_arguments.event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }
    DWORD thr;
    HANDLE thread;
    if ((thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) windows_thread, (LPVOID) (&data_arguments), 0, &thr)) == NULL) {
        fprintf(stderr, "An error Occurred while trying to create the thread.\n");
        fflush(stderr);
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    DWORD out = WaitForSingleObject(data_arguments.event, INFINITE);

    if (out == WAIT_TIMEOUT || out == WAIT_FAILED || out == WAIT_ABANDONED ) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    out = WaitForSingleObject(thread, INFINITE);

    if (out == WAIT_TIMEOUT || out == WAIT_FAILED || out == WAIT_ABANDONED ) {
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    CloseHandle(thread);
#endif
    if (data_arguments.error_out == 1) {
        fprintf(stderr, "An error occurred while trying to execute the command.\n");
        fflush(stderr);
        free(cpy_command);
        free(cpy_args);
        return 1;
    }

    char * response = create_http_response(200, data_arguments.out_size, "text/html; charset=utf-8", NULL, NULL);
    if (response == NULL) {
        free(cpy_command);
        free(cpy_args);
        free(data_arguments.out);
        return 1;
    }

    Send(socket, response, strlen(response), 0);
    Send(socket, data_arguments.out, data_arguments.out_size, 0);

    http_log(http_h,response,address,0);

    free(response);
    free(cpy_command);
    free(cpy_args);
    free(data_arguments.out);

    return 0;
}

int is_dir(char * url) {
#ifdef __unix__
    struct stat st;
    stat(url, &st);
    if (S_IFREG & st.st_mode) {
        return 0;
    }
    if (S_IFDIR & st.st_mode) {
        return 1;
    }
#elif _WIN32
    DWORD attr = GetFileAttributes((LPCTSTR) url );
    if (attr == INVALID_FILE_ATTRIBUTES) {
        LPSTR messageBuffer = NULL;
        //size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    }
    if (attr != INVALID_FILE_ATTRIBUTES &&
         (attr & FILE_ATTRIBUTE_DIRECTORY)) return 1;
#endif
    return 0;
}

char *list_dir(char *dir_name) {
    int buf_size = BUFSIZE;
    int pos = 0;

    char *dirs = malloc(buf_size);
    if (dirs == NULL) return NULL;

#ifdef _WIN32
    WIN32_FIND_DATA ffd;
    int len = strlen(dir_name) + strlen("\\*")+1;
    TCHAR szDir[len];
    HANDLE hFind = INVALID_HANDLE_VALUE;

    memcpy(szDir, dir_name, strlen(dir_name)+1);
    strcat(szDir, TEXT("\\*"));

    hFind = FindFirstFile(szDir, &ffd);

    if (INVALID_HANDLE_VALUE == hFind) {
        return NULL;
    }

    do {
        int written = 0;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            while (strlen(ffd.cFileName) + 1 + strlen("\n") + pos > buf_size-1) {
                buf_size *= 2;
                char * point = realloc(dirs, buf_size);
                if (point == NULL) {
                    buf_size /= 2;
                    continue;
                }
                dirs = point;
            }
            written = snprintf(dirs+pos, buf_size, "%s/\n", ffd.cFileName);
        } else {
            while (strlen(ffd.cFileName) + strlen("\n") + pos > buf_size-1) {
                buf_size *= 2;
                char * point = realloc(dirs, buf_size);
                if (point == NULL) {
                    buf_size /= 2;
                    continue;
                }
                dirs = point;
            }
            written = snprintf(dirs+pos, buf_size, "%s\n", ffd.cFileName);

        }
        pos += written;

    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);
    CloseHandle(hFind);

#elif __unix__
    DIR *d;
    // Open the directory
    d = opendir(dir_name);

    if (d == NULL) {
        return NULL;
    }

    for (;;) {
        struct dirent *entry;
        // Select one file from the directory
        entry = readdir(d);
        if (!entry)
            break;

        int written = 0;

        if (DT_DIR & entry->d_type) {

            while (strlen(entry->d_name) + 1 + strlen("\n") + pos > buf_size-1) {
                buf_size *= 2;
                char * point = realloc(dirs, buf_size);
                if (point == NULL) {
                    buf_size /= 2;
                    continue;
                }
                dirs = point;
            }
            written = snprintf(dirs+pos, buf_size, "%s/\n", entry->d_name);

        } else {
            while (strlen(entry->d_name) + strlen("\n") + pos > buf_size-1) {
                buf_size *= 2;
                char * point = realloc(dirs, buf_size);
                if (point == NULL) {
                    buf_size /= 2;
                    continue;
                }
                dirs = point;
            }
            written = snprintf(dirs+pos, buf_size, "%s\n", entry->d_name);
        }
        pos += written;
    }


#endif
    return dirs;
}

void send_file (int socket, http_header http_h, char * address) {

    char * url = http_h.url;
    if (is_dir(url+1) == 1) {

        char *content = list_dir(url+1);

        if (content == NULL) {
            char * resp = create_http_response(500, -1,NULL, NULL,NULL);
            if (resp == NULL) {
                return;
            }
            Send(socket,resp, strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            return;
        }

        int content_len = strlen(content);
        char * resp = create_http_response(200, content_len,"text/html; charset=utf-8", NULL,NULL);
        if (resp == NULL) {
            free(content);
            return;
        }

        if (Send(socket,resp, strlen(resp), 0) != -1) Send(socket, content, content_len, 0);

        http_log(http_h,resp,address,0);
        free(content);
        free(resp);
        return;
    }

    FILE * pfile;

    pfile = fopen((url+1),"rb");
    if (pfile == NULL) {
        char * resp = create_http_response(404,-1,NULL, NULL, NULL);
        if (resp == NULL) {
            return;
        }

        Send(socket,resp,strlen(resp),0);
        http_log(http_h,resp,address,0);
        free(resp);
        return;
    }

    fseek(pfile, 0, SEEK_END);
    long lengthOfFile = ftell(pfile);
    fseek(pfile, 0, SEEK_SET);

    if (lock_file(pfile,lengthOfFile, 1) == 1){
        fprintf(stderr,"Couldn't acquire lock on file.\n");
        return;
    }

    char * resp = create_http_response(200,lengthOfFile,"text/html; charset=utf-8", get_file_name(url),NULL);
    if (resp == NULL) {
        goto unlock;
    }

    int err = Send(socket,resp, strlen(resp), 0);
    if (err == -1) {
        goto unlock;
    }

    char buff [BUFSIZE];
    int read = 0;
    int remaining_to_read = lengthOfFile;

    for(;;) {
        read = fread(buff,1,BUFSIZE,pfile);
        printf("read: %d\n", read);
        fflush(stdout);
        Send(socket,buff,read,0);

        if (read == remaining_to_read || read == 0) {
            break;
        }
        remaining_to_read -= read;
    }

    http_log(http_h,resp,address,0);

unlock:
    if (unlock_file(pfile,lengthOfFile) == 1) {
        fprintf(stderr,"Couldn't unlock file.\n");
    }
    free(resp);
    fclose(pfile);
}

void put_file (int clientfd, http_header http_h, char * address, char * buffer, const int BUFF_READ_LEN,int header_len, int data_read) {
    FILE * file;
    file = fopen(http_h.url+1, "wb" );

    if (file == NULL) {
        fprintf(stderr,"An error occurred while trying to open file %s.\n", http_h.url +1);
        return;
    }

    if (lock_file(file,http_h.attribute.content_length, 0) == 1) {
        fprintf(stderr,"Couldn't acquire lock on file.\n");
        char * resp = create_http_response(423,-1, NULL, NULL, NULL);
        if (resp == NULL) {
            fclose(file);
            return;
        }
        Send(clientfd,resp,strlen(resp),0);
        http_log(http_h,resp,address,0);
        fclose(file);
        free(resp);
        return;
    }

    char * data = NULL;
    data = buffer + header_len;

    int data_write_in_file = 0;
    int data_l = data_read - header_len;


    if (http_h.attribute.content_length == data_l) {
        // Se ho già letto i dati alla prima lettura con l'header
        int write = fwrite(data,1,data_l,file);

        if (write == 0) {
            char * resp = create_http_response(500,-1, NULL, NULL, NULL);
            if (resp == NULL) {
                goto unlock;
            }

            Send(clientfd, resp,strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            goto unlock;
        }
        // Invio la richiesta di fine.
        char * resp = create_http_response(201,-1, NULL, NULL, http_h.url);
        if (resp == NULL) {
            goto unlock;
        }
        Send(clientfd, resp,strlen(resp), 0);
        http_log(http_h,resp,address,0);
        free(resp);
        goto unlock;
    }

    int write = fwrite(data,1,data_l,file);

    if (write != data_l) {
        // se non riesco a scrivere tutto
        char * resp = create_http_response(500,-1, NULL, NULL, NULL);
        if (resp == NULL) {
            goto unlock;
        }
        Send(clientfd, resp,strlen(resp), 0);
        http_log(http_h,resp,address,0);
        free(resp);
        goto unlock;
    }

    int remaining_to_read = http_h.attribute.content_length - data_l;

    for(;;) {
        int read;
        if (remaining_to_read < BUFF_READ_LEN) read = recv(clientfd,buffer,remaining_to_read, 0);
        else  read = recv(clientfd,buffer,BUFF_READ_LEN, 0);

        if (read == 0 || read == -1) {
            goto unlock;
        }

        write = fwrite(buffer,1,read,file);
        //m_sleep(10);
        if (write == 0) {
            char * resp = create_http_response(500,-1, NULL, NULL, NULL);
            if (resp == NULL) {
                goto unlock;
            }
            Send(clientfd, resp,strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            goto unlock;
        }

        if (read == remaining_to_read) {
            break;
        }
        remaining_to_read -= read;
    }

    char * resp = create_http_response(201,-1, NULL, NULL, http_h.url);
    if (resp == NULL) {
        goto unlock;
    }
    Send(clientfd, resp,strlen(resp), 0);
    http_log(http_h,resp,address,0);
    free(resp);

unlock:
    if (unlock_file(file,http_h.attribute.content_length) == 1) {
        fprintf(stderr,"Couldn't unlock file.\n");
    }
    fclose(file);
}

void encrypt (unsigned int * buff, unsigned int address, int pad) {
    char *ptr = (char *) buff;
    char buff_copy[4] = {0};
    int rem = 4 - pad;
    for (int i = 0; i < rem; i++) {
        // The remaining "pad" bytes are zeroes (padding)
        buff_copy[i] = ptr[i];
    }
    int *buff_copy_i;
    buff_copy_i = (int *) buff_copy;

    int val = rand_r(&address);
    *buff_copy_i = ((unsigned int) (*buff_copy_i)) ^ val;

    for (int i = 0; i < rem; i++) {
        ptr[i] = buff_copy[i];
    }
}

void send_file_chipher (int socket, http_header http_h, unsigned int address, char * conv_address) {

    FILE * file;
    file = fopen(http_h.url+1,"rb");

    if (file == NULL) {
        fprintf(stderr,"An error occurred while trying to open file %s\n", http_h.url +1);
        char *resp = create_http_response(404, -1, NULL, NULL, NULL);
        if (resp == NULL) {
            return;
        }
        Send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        return;
    }

    fseek(file, 0, SEEK_END);
    int lengthOfFile = ftell(file);
    fseek(file, 0, SEEK_SET);
    int last = (lengthOfFile % 4);
    int n_pack = (int)(lengthOfFile / 4);
    int padding = 4 - last;

#ifdef __unix__

    if (lock_file(file,lengthOfFile, 1) == 1) {
        fprintf(stderr,"Couldn't acquire lock on file.\n");
        return;
    }

    char * map;
    int fd = fileno(file);
    map = mmap(0,lengthOfFile,PROT_READ | PROT_WRITE, MAP_PRIVATE,fd,0);

    if (map == MAP_FAILED) {
        char *resp = create_http_response(500, -1, NULL, NULL, NULL);
        if (resp == NULL) {
            goto unlock;
        }
        Send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        goto unlock;
    }
#elif _WIN32
    fclose(file);

    HANDLE file_h = CreateFile(http_h.url+1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_h == INVALID_HANDLE_VALUE) {
        char *resp = create_http_response(404, -1, NULL, NULL, NULL);
        if (resp == NULL) {
            return;
        }
        Send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        return;
    }

    OVERLAPPED sOverlapped = {0};
    if (LockFileEx(file_h, LOCKFILE_EXCLUSIVE_LOCK, 0, lengthOfFile, 0, &sOverlapped) == FALSE) {
        fprintf(stderr,"Couldn't acquire lock on file.\n");
        return;
    }

    HANDLE map_h = CreateFileMapping(file_h, NULL, PAGE_READONLY, 0, lengthOfFile, NULL);

    if (map_h == NULL) {
        char *resp = create_http_response(500, -1, NULL, NULL, NULL);
        if (resp == NULL) {
            CloseHandle(file_h);
            return;
        }
        Send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        CloseHandle(file_h);
        return;
    }

    char * map = MapViewOfFile(map_h, FILE_MAP_COPY, 0, 0, lengthOfFile);

    if (map == NULL) {
        char *resp = create_http_response(500, -1, NULL, NULL, NULL);
        if (resp == NULL) {
            CloseHandle(file_h);
            CloseHandle(map_h);
            return;
        }
        Send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        CloseHandle(map_h);
        CloseHandle(file_h);
        return;
    }
#endif

    unsigned int * map_int;
    map_int = (unsigned int *) map;

    for (int i = 0; i < n_pack; i++) {
        encrypt(map_int + (i) , address, 0);
    }

    encrypt(map_int + n_pack, address, padding);

    char *resp = create_http_response(200, lengthOfFile, "text/html; charset=utf-8", get_file_name(http_h.url+1), NULL);
    if (resp == NULL) {
        goto unmap;
    }

    if (Send(socket, resp, strlen(resp), 0) != -1)
        Send(socket,map,lengthOfFile,0); // The padding is not sent

    http_log(http_h, resp, conv_address, 0);

    free(resp);

unmap:
#ifdef __unix__
    if (munmap(map, lengthOfFile) == -1) {
        fprintf(stderr, "An error occurred while trying to un-mmap file.\n");
    }
unlock:
    if (unlock_file(file, lengthOfFile) == 1) {
        fprintf(stderr,"Couldn't unlock file\n.");
    }
    fclose(file);
#elif __WIN32
    if (UnmapViewOfFile(map) == FALSE) {
        fprintf(stderr, "EAn error occurred while trying to un-mmap file.\n");
    }
    if (UnlockFileEx(file_h, 0, lengthOfFile, 0, &sOverlapped) == FALSE) {
        fprintf(stderr,"Couldn't unlock file\n");
    }
    CloseHandle(map_h);
    CloseHandle(file_h);
unlock:
#endif
    return;
}