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

    //printf("Initialising Child Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        fprintf(stderr,"Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    //printf("Initialised.\n");
    fflush(stdout);

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
    int nRet;
    DWORD dwErr;

    nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen);
    //printf("The needed buffer size is %d bytes...\n", dwBufLen);
    if ((dwErr = WSAGetLastError()) != WSAENOBUFS) {
        fprintf(stderr, "Critical Problem Occurred\n");
        exit(EXIT_FAILURE);
    }
    //printf("Error >expected< %d\nNow allocation space for the structure...\n", WSAGetLastError());
    lpProtocolBuf = (WSAPROTOCOL_INFO *)malloc(dwBufLen);
    if (nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen) == SOCKET_ERROR) {
        fprintf(stderr, "Couldn't allocate space for WSAPROTOCOL_INFO structure\n");
        exit(EXIT_FAILURE);
    }

    DWORD cbRead;
    BOOL fSuccess;

    //Read from Pipe and store result in WSAPROTOCOL_INFO struct
    if (fSuccess = ReadFile(pipe_h, (void *) lpProtocolBuf, 1*dwBufLen, &cbRead, NULL) == FALSE)
        fprintf(stderr, "Couldn't Read from Pipe\n");

    SOCKET sock_fd;
    //Re-create socket using info from WSAPROTOCOL_INFO
    sock_fd = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, lpProtocolBuf, 0, WSA_FLAG_OVERLAPPED);

    fflush(stdout);

    CloseHandle(pipe_h);
    process_routine((void *) &sock_fd);
    //ToDo Disallocare spazio di lpProtocolBuf

#endif
    return 0;
}