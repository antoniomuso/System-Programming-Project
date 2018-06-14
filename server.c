//
// Created by anotoniomusolino on 31/05/18.
//
#include <stddef.h>
#include "b64.c/b64.h"


#include "command_parser.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFF_READ_LEN 8000

#ifdef __unix__

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>




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


/*
 * Run thread function
 */

int set_blocking(int sockfd, int blocking) {
    int nonblock = blocking == 0 ? 1 : 0;
#ifdef __unix__
    return ioctl(sockfd, FIONBIO, &nonblock);
#elif _WIN32
    u_long wnonblock = (u_long) nonblock;
    return ioctlsocket(sockfd, FIONBIO, &wnonblock);
#endif
}


void* process_routine (void *arg) {
    printf("Thread Start\n");
    fflush(stdout);

    int server_socket = *((int*)arg);
    int server_socket_chiper = *( ((int*)arg) + 1 );

    int clientfd;
    //char * header_buffer = malloc(BUFF_READ_LEN);
    char * buffer = malloc(BUFF_READ_LEN);

    set_blocking(server_socket, 0);
    set_blocking(server_socket_chiper, 0);

    fd_set fds;
    FD_ZERO(&fds);

    FD_SET(server_socket,&fds);
    FD_SET(server_socket_chiper,&fds);

    int maxfdp = server_socket > server_socket_chiper ?  server_socket  : server_socket_chiper;

    int rc = 0;
    while ((rc = select(maxfdp + 1 , &fds, NULL, NULL, NULL) ) != -1) {

        //printf("Select active\n");
        //fflush(stdout);
        FD_SET(server_socket,&fds);
        FD_SET(server_socket_chiper,&fds);

        if ((clientfd = accept(server_socket_chiper, NULL, NULL)) > 1) {

        } else if ( (clientfd = accept(server_socket, NULL, NULL)) > 1 ) {

        } else {
            continue;
        }

        set_blocking(clientfd, 1);


        int read_len;
        int data_read = 0;

        http_header http_h;

        while ((read_len = recv(clientfd, (void *)(buffer + data_read),(BUFF_READ_LEN-1) - data_read,0)) == -1 || read_len) {
            buffer[read_len + data_read] = '\0';

            char * pointer = strstr(buffer,"\r\n\r\n");

            if (pointer == NULL) {
                printf("pointer is: NULL\n");
                fflush(stdout);
                data_read += read_len;
                continue;
                //break;
            }

            pointer += 4;
            int header_len = pointer - buffer;

            http_h = parse_http_header_request(buffer, header_len);

            if (http_h.is_request < 0) {
                fprintf(stderr,"Error HTTP parse");
                break;
            }
            if (http_h.attribute.authorization != NULL) {
                printf("Auth: %s\n", http_h.attribute.authorization);
            }
            printf("%s %s %s\n",http_h.type_req,http_h.url, http_h.attribute.user_agent);
            fflush(stdout);
#ifdef __unix__
            close(clientfd);
#elif _WIN32
            closesocket(clientfd);
#endif
            break;
        }
    }
}
int w_process_routine (void *arg) {
    process_routine(arg);
    return 0;
}

int infanticide(void *children_array, int len, int mode, int exit_code) {
    /**
     * Mode: 0 = MT, 1 = MP.
     */
     int i = 0;
#ifdef _WIN32
    HANDLE *array = (HANDLE *) children_array;
    if (mode == 0) {
        for (i = 0; i < len; i++) {
            if(!TerminateThread(array[i], exit_code))
                return i;
        }
    } else if (mode == 1) {
        for (i = 0; i < len; i++) {
            if(!TerminateProcess(array[i], exit_code))
                return i;
        }
    }
#elif __unix__

    if (mode == 0) {
        pthread_t *tids = (pthread_t *) children_array;

        for (i = 0; i < len; i++) {
            int err = 0;
            if (err = pthread_kill(tids[i], exit_code) != 0) {
                fprintf(stderr,"%s\n", strerror(err));
                return i;
            }
        }
    } else if (mode == 1) {
        int *pids = (int *) children_array;

        for (i = 0; i < len; i++) {
            if (kill(pids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return i;
            }
        }
    }
    
#endif
    return i;
}

int run_server(options c_options, options f_options) {

#if _WIN32
    WSADATA wsa;

    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        fprintf(stderr,"Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

#endif

    struct addrinfo *addr_info;
    struct addrinfo *addr_info_chipher;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int server_socket_cipher = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;

    char *port = get_command_value("-port",c_options) != NULL
                 ? get_command_value("-port",c_options)
                 : get_command_value("port", f_options);

    char *server_ip = get_command_value("-server_ip", c_options) != NULL
                      ? get_command_value("-server_ip", c_options)
                      : get_command_value("server_ip", f_options);

    char *mode = get_command_value("-mode", c_options) != NULL
                 ? get_command_value("-mode", c_options)
                 : get_command_value("mode", f_options);

    int n_proc = get_command_value("-n_proc", c_options) != NULL
                 ? atoi(get_command_value("-n_proc", c_options))
                 : atoi(get_command_value("n_proc", f_options));
    char chiper_port[MAX_OPTION_LEN];
    snprintf(chiper_port, MAX_OPTION_LEN, "%d", atoi(port) + 1);

    if (n_proc <= 0) {
        fprintf(stderr, "Error n_proc <= 0");
        exit(EXIT_FAILURE);
    }


    if (server_socket == -1 || server_socket_cipher == -1) {
        fprintf(stderr,"Couldn't create socket\n");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)& yes, sizeof(int)) == -1
        || setsockopt(server_socket_cipher, SOL_SOCKET, SO_REUSEADDR, (const void *)& yes, sizeof(int)) == -1) {
        fprintf(stderr,"Couldn't setsockopt\n");
        exit(EXIT_FAILURE);
    }

    /* Fill the address info struct (host + port) -- getaddrinfo(3) */


    if (getaddrinfo(server_ip, port, NULL, &addr_info) != 0
        || getaddrinfo(server_ip, chiper_port, NULL, &addr_info_chipher) != 0 ) {
        fprintf(stderr,"Couldn't get address\n");
        exit(EXIT_FAILURE);

    }

    if (bind(server_socket, addr_info->ai_addr, addr_info->ai_addrlen) != 0
        || bind(server_socket_cipher, addr_info_chipher->ai_addr, addr_info_chipher->ai_addrlen) != 0) {
        fprintf(stderr,"Couldn't bind sockets to address\n");
        exit(EXIT_FAILURE);
    }

    /* Free the memory used by our address info struct */
    freeaddrinfo(addr_info);
    freeaddrinfo(addr_info_chipher);


    if (listen(server_socket, 10) == -1
        || listen(server_socket_cipher, 10) == -1 ) {
        fprintf(stderr,"Couldn't make sockets listen\n");
        exit(EXIT_FAILURE);
    }


    printf("Server listening on %s:%s\n", server_ip, port);
    printf("Server chiper listening on %s:%s\n", server_ip, chiper_port);
    fflush(stdout);



#ifdef __unix__

    if (strcmp(mode, "MT") == 0) {
        printf("mode = MT\n");
        fflush(stdout);

        pthread_t tid[n_proc - 1];

        int sockets[2];
        sockets[0] = server_socket;
        sockets[1] = server_socket_cipher;

        int *sock_pointer;
        sock_pointer = sockets;

        for (int i = 0; i < n_proc - 1; i++) {
            pthread_create(&(tid[i]), NULL, &process_routine, (void *) sock_pointer);
        }

        free_options(c_options);
        free_options(f_options);
        process_routine(sock_pointer);

    } else if (strcmp(mode, "MP") == 0) {
        //TODO Riordarsi di uccidere i figli quando finiscono o in caso di fallimento di qualsiasi operazione.
        printf("mode = MP\n");
        fflush(stdout);

        int pids[n_proc - 1];

        int sockets[2];
        sockets[0] = server_socket;
        sockets[1] = server_socket_cipher;

        int *sock_pointer;
        sock_pointer = sockets;

        for (int i = 0; i < n_proc-1; i++) {
            int pid = fork();
            if (pid > 0) {
                pids[i] = pid;
            } else if (pid == 0) {
                process_routine((void *)sock_pointer);
            } else {
                    fprintf(stderr, "Fork Error %i", pid);
                    exit(EXIT_FAILURE);
            }

        }

        free_options(c_options);
        free_options(f_options);
        process_routine(sock_pointer);

    }

#elif _WIN32


    if (strcmp(mode, "MT") == 0) {
        HANDLE hThreadArray[n_proc-1];
        DWORD dwThreadArray[n_proc-1];

        int *sock_pointer;
        sock_pointer = &server_socket;

        for (int i = 0; i < n_proc-1; i++) {
            hThreadArray[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) w_process_routine, (LPVOID) sock_pointer, 0, &dwThreadArray[i]);
        }

        free_options(c_options);
        free_options(f_options);
        process_routine(sock_pointer);
    } else if (strcmp(mode, "MP") == 0) {
        /**
         * TODO: Ricordarsi di chiudere i processi una volta finito o in caso di fallimento di qualsiasi operazione
         */

        int buf_size = 30;

        char buff[buf_size];


        //STARTUPINFO startup_info[n_proc-1];
        //PROCESS_INFORMATION proc_info[n_proc-1];

        STARTUPINFO startup_info = {0};
        PROCESS_INFORMATION proc_info = {0};

        HANDLE children_handle[n_proc-1];

        char *pipe_n = "\\\\.\\pipe\\testpipe";

        for (int i = 0; i < n_proc-1; i++) {

            snprintf(buff, buf_size, "%d", i);

            int len_name = strlen(pipe_n)+strlen(buff)+1;
            char buffer_name[len_name];
            snprintf(buffer_name, len_name, "%s%d", pipe_n, i);

            LPTSTR pipe_name = TEXT(buffer_name);

            HANDLE pipe_h = CreateNamedPipe(
                    pipe_name,
                    PIPE_ACCESS_OUTBOUND,
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES,
                    2*sizeof(WSAPROTOCOL_INFO),
                    0,
                    0,
                    NULL);


            if (pipe_h == INVALID_HANDLE_VALUE) {
                printf("Couldn't create Pipe\n");
                exit(EXIT_FAILURE);
            }

            if (!(CreateProcess("windows_process_exe.o", buff, NULL, NULL, FALSE, 0, NULL, NULL, &(startup_info), &(proc_info) ))) {
                fprintf(stderr, "Error occurred while trying to create a process\n");
                fflush(stderr);
                exit(EXIT_FAILURE);
            }

            DWORD proc_pid = proc_info.dwProcessId;
            children_handle[i] = proc_info.hProcess;

            WSAPROTOCOL_INFO wsa_prot_info[2];

            WSAPROTOCOL_INFO wsa_prot_info_1;
            WSAPROTOCOL_INFO wsa_prot_info_2;

            //Duplicate socket
            if ((WSADuplicateSocket(server_socket, proc_pid, &(wsa_prot_info_1) ) == SOCKET_ERROR)
                    || WSADuplicateSocket(server_socket_cipher, proc_pid, &(wsa_prot_info_2) ) == SOCKET_ERROR) {
                fprintf(stderr, "Error occurred while trying to duplicate socket %d for process %d (%d))\n", server_socket, proc_pid, WSAGetLastError());
                exit(EXIT_FAILURE);
            }


            BOOL fSuccess = FALSE;
            //Wait for child to connect to pipe
            fSuccess = ConnectNamedPipe(pipe_h, NULL) ? TRUE : FALSE;
            if (!fSuccess) {
                fprintf(stderr, "No incoming child connection\n");
                exit(EXIT_FAILURE);
            }
            DWORD written = 0;
            if (WriteFile(pipe_h, &wsa_prot_info_1, sizeof(WSAPROTOCOL_INFO), &written, NULL) == FALSE)
                fprintf(stderr, "Couldn't write to pipe\n");
            if (WriteFile(pipe_h, &wsa_prot_info_2, sizeof(WSAPROTOCOL_INFO), &written, NULL) == FALSE)
                fprintf(stderr, "Couldn't write to pipe\n");
            fflush(stdout);
        }

        free_options(c_options);
        free_options(f_options);

        int server_socket_arr[2];
        server_socket_arr[0] = server_socket;
        server_socket_arr[1] = server_socket_cipher;
        int *server_socket_ptr;
        server_socket_ptr = server_socket_arr;
        process_routine((void *) server_socket_ptr);
    }


#endif

}

