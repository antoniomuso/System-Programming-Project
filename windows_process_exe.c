#include <stddef.h>
#include "b64.c/b64.h"
#include "command_parser.h"
#include "server.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef  _WIN32
# undef  _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_WINXP
# undef  WINVER
# define WINVER       _WIN32_WINNT_WINXP

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws_32.lib")
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        fprintf(stderr,"WSAStartup failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    char *pipe_n = "\\\\.\\pipe\\testpipe";
    int len_name = strlen(pipe_n)+strlen(argv[0])+1;
    char buffer_name[len_name];
    snprintf(buffer_name, len_name, "%s%s", pipe_n, argv[0]);

    LPTSTR pipe_name = TEXT(buffer_name);
    HANDLE pipe_h;

    while (1) {
        //Try to connect to Pipe
        pipe_h = CreateFile(pipe_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, NULL);
        if (pipe_h != INVALID_HANDLE_VALUE) break;
    }

    //Initialize and allocate WSAPROTOCOL_INFO struct
    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;

    DWORD dwBufLen = 0;
    DWORD dwErr;

    // WSAEnumProtocols()'s Failure is expected here. Its purpose here is to initialize dwBufLen
    if ((WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen) != SOCKET_ERROR))
        printf("WSAEnumProtocols() failed with error code %d\n", WSAGetLastError());
    if ((dwErr = WSAGetLastError()) != WSAENOBUFS) {
        fprintf(stderr, "Critical Error Occurred\n");
        exit(EXIT_FAILURE);
    }

    DWORD dwBufLens = dwBufLen*2;
    lpProtocolBuf = (WSAPROTOCOL_INFO *)malloc(dwBufLens);
    if (lpProtocolBuf == NULL) {
        exit(EXIT_FAILURE);
    }

    if (WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLens) == SOCKET_ERROR) {
        fprintf(stderr, "Couldn't allocate space for WSAPROTOCOL_INFO structure\n");
        free(lpProtocolBuf);
        exit(EXIT_FAILURE);
    }

    DWORD cbRead_1;
    DWORD cbRead_2;
    BOOL fSuccess;

    //Read from Pipe and store result in WSAPROTOCOL_INFO struct
    if ((fSuccess = ReadFile(pipe_h, (void *) lpProtocolBuf, 1*dwBufLens, &cbRead_1, NULL) == FALSE)) {
        fprintf(stderr, "Couldn't Read from Pipe\n");
        free(lpProtocolBuf);
    }

    SOCKET sock_fd[2];

    //Re-create socket using info from WSAPROTOCOL_INFO
    sock_fd[0] = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, lpProtocolBuf, 0, WSA_FLAG_OVERLAPPED);
    sock_fd[1] = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, lpProtocolBuf+1, 0, WSA_FLAG_OVERLAPPED);


    if ((sock_fd[0] == INVALID_SOCKET) || sock_fd[1] == INVALID_SOCKET) {
        printf("WSASocket() failed with error code %d\n", WSAGetLastError());
        free(lpProtocolBuf);
        WSACleanup();
    }

    free(lpProtocolBuf);
    CloseHandle(pipe_h);

    SOCKET *sock_fd_ptr;
    sock_fd_ptr = sock_fd;
    launch_mode = 1;
    process_routine((void *) sock_fd_ptr);
#endif
    return 0;
}