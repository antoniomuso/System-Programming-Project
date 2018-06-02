//
// Created by anotoniomusolino on 31/05/18.
//


#include "command_parser.h"
#include "server.h"
#include <stdio.h>

#ifdef __unix__

#include <sys/socket.h>


#elif _WIN32

#include <winsock2.h>

#endif


int run_server(options options) {
    printf("Server start\n");

#if _WIN32
    WSADATA wsa;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        fprintf(stderr,"Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }



    printf("Initialised.");

#endif



    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;

    if (server_socket == -1) {
        fprintf(stderr,"Couldn't create socket");
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)& yes, sizeof(int)) == -1) {
        fprintf(stderr,"Couldn't setsockopt");
    }






}