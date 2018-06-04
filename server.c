//
// Created by anotoniomusolino on 31/05/18.
//
#include <stddef.h>
#include "b64.c/b64.h"


#include "command_parser.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>


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

#endif

/*
 * Run thread function
 */
void* process_routine () {
    printf("Thread Start\n");
    fflush(stdout);
}

int run_server(options options) {

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

    if (server_socket == -1) {
        fprintf(stderr,"Couldn't create socket\n");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)& yes, sizeof(int)) == -1) {
        fprintf(stderr,"Couldn't setsockopt\n");
        exit(EXIT_FAILURE);
    }

    /* Fill the address info struct (host + port) -- getaddrinfo(3) */


    if (getaddrinfo(get_command_value("-server_ip", options), get_command_value("-port",options), NULL, &addr_info) != 0) {
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


    printf("Server Listen on %s:%s\n", get_command_value("-server_ip", options), get_command_value("-port", options));
    fflush(stdout);

    /*
    int clientfd;
    while (clientfd = accept(server_socket, NULL, NULL)) {
        printf("Client Connect\n");
        fflush(stdout);
        send(clientfd,"First Message",14,0);
        //close(server_socket);
    }
    */

#ifdef __unix__

    int n_proc = atoi(get_command_value("-n_proc", options));
    pthread_t tid[n_proc];


    for (int i = 0; i < n_proc; i++) {
        pthread_create(&(tid[i]), NULL, &process_routine, NULL);
    }

#elif _WIN32

#endif

}

