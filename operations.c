//
// Created by anotoniomusolino on 18/06/18.
//

#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#define TIME_WAIT 6000
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

#endif

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
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
#endif
};

int log_write(char *cli_addr, char *user_id, char *username, char *request, char * url,
              char * protocol_type, int return_code, int bytes_sent) {

    FILE *logfile = fopen(LOGFILE, "a");
    if (logfile == NULL) {
        fprintf(stderr, "Unable to open logfile");
        exit(EXIT_FAILURE);
    }

#ifdef __unix__
    int fd = fileno(logfile);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Error during file lock\n");
        // return response with error lock
        fclose(logfile);
        return 0;
    }
#endif
    int timestr_len = 27;
    char timestamp_str[timestr_len];

//    tzset(); //Controllare se serve su unix
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
        exit(EXIT_FAILURE);
    }

#ifdef __unix__
    while (flock(fd,LOCK_UN) != 0) {
        sleep(100);
    }
#endif

    fclose(logfile);
    return 1;
}

int http_log (http_header h_request, char * h_response, char * client_address, int no_name) {
    http_header h_resp = parse_http_header_response(h_response,strlen(h_response));
    if (h_resp.is_request == -1) {
        fprintf(stderr, "Error during response parsing");
        return 0;
    }
    int err = 0;
    if (!no_name) {
        if (h_request.attribute.authorization == NULL) return 0;
        authorization auth = parse_authorization(h_request.attribute.authorization);
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
    if (waitpid(pid,&status,0) < 0  && errno != ECHILD) {
        fprintf(stderr, "Error in waitpid erro: %s\n", strerror(errno));
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

int exec_command(int socket, const char * command, const char * args, http_header http_h, char * address) {
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

    http_log(http_h,response,address,0);

    free(response);
    free(cpy_command);
    free(cpy_args);
    free(data_arguments->out);
    free(data_arguments);

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

#elif __unix__
    DIR *d;
    // Apro directory
    d = opendir(dir_name);

    if (d == NULL) {
        return NULL;
    }

    for (;;) {
        struct dirent *entry;
        // leggo un file dalla directory
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
            send(socket,resp, strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            return;
        }

        int content_len = strlen(content);
        char * resp = create_http_response(200, content_len,"text/html; charset=utf-8", NULL,NULL);
        if (send(socket,resp, strlen(resp), 0) != -1)
            send(socket, content, content_len, 0);

        http_log(http_h,resp,address,0);
        free(content);
        free(resp);
        return;
    }


    FILE * pfile;

    pfile = fopen((url+1),"rb");
    if (pfile == NULL) {
        //Send a error response
        char * resp = create_http_response(404,-1,NULL, NULL, NULL);
        send(socket,resp,strlen(resp),0);
        http_log(http_h,resp,address,0);
        free(resp);
        return;
    }

#ifdef __unix__
    int fd = fileno(pfile);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Error during file lock\n");
        // return response with error lock
        fclose(pfile);
        return;
    }
#endif

    fseek(pfile, 0, SEEK_END);
    int lengthOfFile = ftell(pfile);
    fseek(pfile, 0, SEEK_SET);

    char * resp = create_http_response(200,lengthOfFile,"text/html; charset=utf-8", get_file_name(url),NULL);
    int err = send(socket,resp, strlen(resp), 0);
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
        send(socket,buff,read,0);

        if (read == remaining_to_read || read == 0) {
            break;
        }
        remaining_to_read -= read;
    }
#ifdef __unix__
unlock:
    while (flock(fd,LOCK_UN) != 0) {
        sleep(100);
    }
#endif
    http_log(http_h,resp,address,0);

    free(resp);
    fclose(pfile);
}

void put_file (int clientfd, http_header http_h, char * address, char * buffer, const int BUFF_READ_LEN,int header_len, int data_read) {
    fflush(stdout);
    FILE * file;

    //TODO: Se non si sovrascrive il file decommentare e gestire risposta.
    /*
    file = fopen(http_h.url+1, "r" );
    if (file != NULL) {
        // send a error file exist.
        fclose(file);

        return;
    }
     */
    file = fopen(http_h.url+1, "w" );

#ifdef __unix__
    int fd = fileno(file);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Error during file lock\n");
        // return response with error lock
        fclose(file);
        return;
    }
#endif

    char * data = NULL;
    data = buffer + header_len;

    int data_write_in_file = 0;
    int data_l = data_read - header_len;

    printf("content length: %d\n", http_h.attribute.content_length);
    printf("%d\n",data_l);

    if (http_h.attribute.content_length == data_l) {

        int write = fwrite(data,1,data_l,file);
        if (write == 0) {
            char * resp = create_http_response(500,-1, NULL, NULL, NULL);
            send(clientfd, resp,strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            goto unlock;
        }
        // Invio la richiesta di fine.
        char * resp = create_http_response(201,-1, NULL, NULL, http_h.url);
        send(clientfd, resp,strlen(resp), 0);
        http_log(http_h,resp,address,0);
        free(resp);
        goto unlock;
    }

    int write = fwrite(data,1,data_l,file);
    if (write != data_l) {
        char * resp = create_http_response(500,-1, NULL, NULL, NULL);
        send(clientfd, resp,strlen(resp), 0);
        http_log(http_h,resp,address,0);
        free(resp);
        goto unlock;
    }

    int remaining_to_read = http_h.attribute.content_length - data_l;

    for(;;) {
        int read;
        if (remaining_to_read < BUFF_READ_LEN) read = recv(clientfd,buffer,remaining_to_read, 0);
        else  read = recv(clientfd,buffer,BUFF_READ_LEN, 0);

        printf("read: %d\n", read);
        fflush(stdout);

        write = fwrite(buffer,1,read,file);
        if (write == 0) {
            char * resp = create_http_response(500,-1, NULL, NULL, NULL);
            send(clientfd, resp,strlen(resp), 0);
            http_log(http_h,resp,address,0);
            free(resp);
            goto unlock;
        } // gestisco l'errore

        if (read == remaining_to_read) {
            break;
        }
        remaining_to_read -= read;
    }

    // mando la risposta
    char * resp = create_http_response(201,-1, NULL, NULL, http_h.url);
    send(clientfd, resp,strlen(resp), 0);
    http_log(http_h,resp,address,0);
    free(resp);

#ifdef __unix__
unlock:
    while (flock(fd,LOCK_UN) != 0) {
        sleep(100);
    }
#elif _WIN32
unlock:
#endif
    fclose(file);
}

void encrypt (unsigned int * buff, unsigned int address) {
    int val = rand_r(&address);
    *buff = ((unsigned int) (*buff)) ^ val;
}

void send_file_chipher (int socket, http_header http_h, unsigned int address, char * conv_address) {

    FILE * file;
    file = fopen(http_h.url+1,"rb");

    if (file == NULL) {
        char *resp = create_http_response(404, -1, NULL, NULL, NULL);
        send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        return;
    }

    fseek(file, 0, SEEK_END);
    int lengthOfFile = ftell(file);
    fseek(file, 0, SEEK_SET);

#ifdef __unix__
    int fd = fileno(file);
    if (flock(fd,LOCK_EX) != 0) {
        fprintf(stderr,"Error during file lock\n");
        // return response with error lock
        fclose(file);
        return;
    }

    int last = (lengthOfFile % 4);
    int n_pack = (int)(lengthOfFile / 4);
    int padding = 4 - last;

    char * map;
    map = mmap(0,lengthOfFile + padding,PROT_READ | PROT_WRITE, MAP_PRIVATE,fd,0);

    if (map == MAP_FAILED) {
        char *resp = create_http_response(500, -1, NULL, NULL, NULL);
        send(socket, resp, strlen(resp), 0);
        http_log(http_h, resp, conv_address, 0);
        free(resp);
        goto unlock;
    }


    for (int i = lengthOfFile; i < lengthOfFile + padding; i++) {
        map[i] = 0;
    }

    unsigned int * map_int;

    map_int = (unsigned int *) map;

    n_pack = last != 0 ? n_pack + 1 : n_pack;

    for (int i = 0; i < n_pack; i++) {
        encrypt(map_int + (i) , address);
    }


    char *resp = create_http_response(200, lengthOfFile + padding, "text/html; charset=utf-8", get_file_name(http_h.url+1), NULL);
    send(socket, resp, strlen(resp), 0);
    http_log(http_h, resp, conv_address, 0);
    send(socket,map,lengthOfFile + padding,0);

    free(resp);

unlock:
    while (flock(fd,LOCK_UN) != 0) {
        sleep(100);
    }
#endif


}