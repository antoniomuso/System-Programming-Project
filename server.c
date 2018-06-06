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

#ifdef __unix__

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>



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


void* process_routine (void *arg) {
    printf("Thread Start\n");
    fflush(stdout);

    int server_socket = *((int*)arg);

    int clientfd;

    while (clientfd = accept(server_socket, NULL, NULL)) {
        printf("Client Connect\n");
        fflush(stdout);
        send(clientfd,"First Message",14,0);
        //close(server_socket);
        //sleep(40);
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

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
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


    if (n_proc <= 0) {
        fprintf(stderr, "Error n_proc <= 0");
        exit(EXIT_FAILURE);
    }


    if (server_socket == -1) {
        fprintf(stderr,"Couldn't create socket\n");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)& yes, sizeof(int)) == -1) {
        fprintf(stderr,"Couldn't setsockopt\n");
        exit(EXIT_FAILURE);
    }

    /* Fill the address info struct (host + port) -- getaddrinfo(3) */


    if (getaddrinfo(server_ip, port, NULL, &addr_info) != 0) {
        fprintf(stderr,"Couldn't get address\n");
        exit(EXIT_FAILURE);

    }

    if (bind(server_socket, addr_info->ai_addr, addr_info->ai_addrlen) != 0) {
        fprintf(stderr,"Couldn't bind socket to address\n");
        exit(EXIT_FAILURE);
    }

    /* Free the memory used by our address info struct */
    freeaddrinfo(addr_info);


    if (listen(server_socket, 10) == -1) {
        fprintf(stderr,"Couldn't make socket listen\n");
        exit(EXIT_FAILURE);
    }


    printf("Server Listen on %s:%s\n", server_ip, port);
    fflush(stdout);



#ifdef __unix__

    if (strcmp(mode, "MT") == 0) {
        printf("mode = MT\n");
        fflush(stdout);

        pthread_t tid[n_proc - 1];

        int *sock_pointer;
        sock_pointer = &server_socket;

        for (int i = 0; i < n_proc - 1; i++) {
            pthread_create(&(tid[i]), NULL, &process_routine, (void *) sock_pointer);
        }

        process_routine(sock_pointer);

    } else if (strcmp(mode, "MP") == 0) {
        printf("mode = MP\n");
        fflush(stdout);

        int tid[n_proc - 1];

        int *sock_pointer;
        sock_pointer = &server_socket;

        for (int i = 0; i < n_proc-1; i++) {
            int pid = fork();
            if (pid > 0) {
                tid[i] = pid;
                process_routine((void *)sock_pointer);

            } else if (pid < 0) {
                fprintf(stderr, "Fork Error %i", pid);
                exit(EXIT_FAILURE);
            }
        }

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
        process_routine(sock_pointer);
    } else if (strcmp(mode, "MP") == 0) {
        //if (CreateProcess("windows_process_exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL))
    }


#endif

}

