//
// Created by anotoniomusolino on 31/05/18.
//

#include "signals.h"
#include "command_parser.h"
#include "server.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_READ_LEN 8000
#define MAX_READ_COUNTER 2
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
#include <netinet/in.h>
#include <arpa/inet.h>




#elif _WIN32

# undef  _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_WINXP
# undef  WINVER
# define WINVER       _WIN32_WINNT_WINXP

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws_32.lib")
#include <windows.h>

#define getpid GetCurrentProcessId
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


int close_socket(int socketfd) {
#ifdef __unix__
    return close(socketfd);
#elif _WIN32
    return closesocket(socketfd);
#endif
}


int is_authorize(http_header http_h, options credentials) {
    // gestire la richiesta se authorization è NULL con una risposta 401
    if (http_h.attribute.authorization != NULL) {
        printf("Auth: %s\n", http_h.attribute.authorization);
        authorization auth = parse_authorization(http_h.attribute.authorization);
        printf("name: %s, password: %s\n", auth.name, auth.password);

        if (strlen(auth.name) < MAX_OPTION_LEN
            && strlen(auth.password) < MAX_OPTION_LEN
            && contains(auth.name,auth.password,credentials) == 1 ) {
            free(auth.free_pointer);
            printf("Credentials correct\n");
            return 1;
        }
        printf("Credentials mismatch\n");
        free(auth.free_pointer);
        return 0;

    }
    return 0;

}

void* process_routine (void *arg) {
    printf("Thread Start\n");
    fflush(stdout);

    int server_socket = *((int*)arg);
    int server_socket_chiper = *( ((int*)arg) + 1 );

    options credentials = parse_file("passwordFile.txt", NULL, 0);

    //printf("%d %d \n", server_socket, server_socket_chiper);
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

        struct sockaddr_in saddr = {0};
        size_t addr_len = sizeof(saddr);

        int is_chipher = 0;

        if ((clientfd = accept(server_socket_chiper,(struct sockaddr *) &saddr,(socklen_t *) &addr_len )) > 1) {
            is_chipher = 1;
        } else if ((clientfd = accept(server_socket,(struct sockaddr *) &saddr,(socklen_t*) &addr_len)) > 1 ) {
            is_chipher = 0;
        } else {
            continue;
        }

        //printf("Pid Accept Request: %d\n",getpid());
        printf("Client Connect\n");
        fflush(stdout);
        char * address = inet_ntoa(saddr.sin_addr);

        set_blocking(clientfd, 1);

        int read_len;
        int data_read = 0;

        http_header http_h;
        int read_counter = 0;

        for (;;) {

            if ((read_len = recv(clientfd, (void *)(buffer + data_read),(BUFF_READ_LEN-1) - data_read,0)) == -1 || read_len == 0) {
                close_socket(clientfd);
                printf("exit\n");
                fflush(stdout);
                break;
            }

            buffer[read_len + data_read] = '\0';

            char * pointer = strstr(buffer,"\r\n\r\n");

            if (pointer == NULL) {
                if ( ++read_counter >= MAX_READ_COUNTER ) {
                    char * resp = create_http_response(400,-1, NULL, NULL, NULL);
                    send(clientfd, resp,strlen(resp), 0);
                    close_socket(clientfd);
                    free(resp);
                }
                data_read += read_len;
                continue;
            }

            pointer += 4;
            int header_len = pointer - buffer;

            http_h = parse_http_header_request(buffer, header_len);

            // Errore durante il parsing della richiesta http
            if (http_h.is_request < 0) {
                fprintf(stderr,"Error HTTP parse");
                char * resp = create_http_response(400,-1, NULL, NULL, NULL);
                http_log(http_h, resp,address,1);
                send(clientfd, resp,strlen(resp), 0);
                free(resp);
                close_socket(clientfd);
                break;
            }

            // Controllo se password e username sono corretti.
#define DEBUG 0
#if DEBUG != 1
            if (!is_authorize(http_h,credentials)) {
                char *resp = create_http_response(401,0,NULL, NULL, NULL);
                send(clientfd,resp,strlen(resp),0);
                http_log(http_h,resp,address,1);
                free(resp);
                close_socket(clientfd);
                break;
            }
#endif

            if (strcmp(http_h.type_req, "GET") == 0) {


                //log_write("192.168.0.1", NULL, NULL, http_h.type_req, 0, 0);


                if (is_chipher == 1 ) {
                    // Se siamo in modalità cifratura

                } else if (startsWith("/command/", http_h.url)) {
                    // Siamo in modalità comando

                    struct operation_command op = parser_operation(http_h.url);
                    if (op.comm != NULL) {

                        if(exec_command(clientfd, op.comm, op.args, http_h, address) == 1) {
                            char * resp = create_http_response(500,-1, NULL, NULL, NULL);
                            send(clientfd, resp,strlen(resp), 0);
                            http_log(http_h,resp,address,0);
                            free(resp);
                        }

                    } else {
                        fprintf(stderr, "Command not passed\n");
                        char * resp = create_http_response(400,-1, NULL, NULL, NULL);
                        send(clientfd, resp,strlen(resp), 0);
                        http_log(http_h,resp,address,0);
                        free(resp);
                    }
                    free_operation_command(op);


                } else {

                    printf("url: %s\n", http_h.url);
                    send_file(clientfd,http_h,address);

                }


            } else { // this is PUT



            }

            free_http_header(http_h);
            close_socket(clientfd);
            break;
        }
    }
}
int w_process_routine (void *arg) {
    process_routine(arg);
    return 0;
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

    int *sock_pointer = NULL;

#ifdef __unix__

    if (strcmp(mode, "MT") == 0) {
        printf("mode = MT\n");
        fflush(stdout);

        pthread_t tid[n_proc ];

        int sockets[2];
        sockets[0] = server_socket;
        sockets[1] = server_socket_cipher;

        sock_pointer = sockets;

        for (int i = 0; i < n_proc ; i++) {
            if (pthread_create(&(tid[i]), NULL, &process_routine, (void *) sock_pointer) != 0) {
                fprintf(stderr, "Error during threads creation");
                exit(EXIT_FAILURE);
            }
        }

        set_signal_handler(tid, sizeof(pthread_t), n_proc, 0);

    } else if (strcmp(mode, "MP") == 0) {
        //TODO Riordarsi di uccidere i figli quando finiscono o in caso di fallimento di qualsiasi operazione.
        printf("mode = MP\n");
        fflush(stdout);

        int pids[n_proc ];

        int sockets[2];
        sockets[0] = server_socket;
        sockets[1] = server_socket_cipher;

        sock_pointer = sockets;

        for (int i = 0; i < n_proc; i++) {
            int pid = fork();
            if (pid > 0) {
                pids[i] = pid;
                printf("fork pid: %d\n",pid);
            } else if (pid == 0) {
                free_options(c_options);
                free_options(f_options);

                process_routine((void *)sock_pointer);
            } else {
                    fprintf(stderr, "Fork Error %i", pid);
                    exit(EXIT_FAILURE);
            }

        }
        set_signal_handler(pids, sizeof(int), n_proc, 1);

    } else {
        fprintf(stderr, "Error invalid mode");
        exit(EXIT_FAILURE);
    }

#elif _WIN32


    if (strcmp(mode, "MT") == 0) {
        HANDLE hThreadArray[n_proc];
        DWORD dwThreadArray[n_proc];

        int sockets[2];
        sockets[0] = server_socket;
        sockets[1] = server_socket_cipher;

        sock_pointer = sockets;

        for (int i = 0; i < n_proc; i++) {
            hThreadArray[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) w_process_routine, (LPVOID) sock_pointer, 0, &dwThreadArray[i]);
            if (hThreadArray[i] == NULL) {
                fprintf(stderr, "Error during threads creation");
                exit(EXIT_FAILURE);
            }
        }

        set_signal_handler(hThreadArray, sizeof(HANDLE), n_proc, 0);

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

        HANDLE children_handle[n_proc];

        char *pipe_n = "\\\\.\\pipe\\testpipe";

        for (int i = 0; i < n_proc; i++) {

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

        set_signal_handler(children_handle, sizeof(HANDLE), n_proc, 1);

        int server_socket_arr[2];
        server_socket_arr[0] = server_socket;
        server_socket_arr[1] = server_socket_cipher;

        sock_pointer = server_socket_arr;

    } else {
        fprintf(stderr, "Error invalid mode");
        exit(EXIT_FAILURE);
    }




#endif
    //printf("Father is entering in Loop\n");
    //fflush(stdout);

    free_options(c_options);
    free_options(f_options);

    //process_routine((void *) sock_pointer);
    while(flag_restart == 0) {
#ifdef __unix__
        sleep(1);
#elif _WIN32
        Sleep(1000);
#endif

    }
    flag_restart = 0;

    close_socket(server_socket);
    close_socket(server_socket_cipher);
#ifdef __unix__
    sleep(2);
#elif _WIN32
    Sleep(2000);
#endif
    return 0;


}

