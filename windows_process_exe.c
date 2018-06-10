#include "command_parser.h"
#include "server.h"


#include <stdio.h>
#include <stdlib.h>

#ifdef  _WIN32
# undef  _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_WINXP
# undef  WINVER
# define WINVER       _WIN32_WINNT_WINXP

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws_32.lib")
#include <windows.h>
//
//SOCKET ConvertProcessSocket(SOCKET oldsocket, DWORD source_pid) {
//    HANDLE source_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, source_pid);
//    printf("%d\n", source_handle);
//
//    HANDLE newhandle;
//    if(DuplicateHandle(source_handle, (HANDLE)oldsocket, GetCurrentProcess(), &newhandle, 0, TRUE, DUPLICATE_SAME_ACCESS)) printf("success\n");
//    //CloseHandle(source_handle);
//    fflush(stdout);
//    return (SOCKET)newhandle;
//}
#endif

int main(int argc, char* argv[]) {
    printf("ciao\n");
    fflush(stdout);
#ifdef _WIN32
    WSADATA wsa;

    printf("Initialising Child Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        fprintf(stderr,"Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");
    fflush(stdout);
    //int sock_fd = atoi(argv[0]);
    //int parent_id = atoi(argv[1]);

    //printf("%d %d\n", sock_fd, parent_id);
    //fflush(stdout);

    //int *sock_ptr;
    //*sock_ptr = ConvertProcessSocket(sock_fd, parent_id);
    //printf("%d\n", *sock_ptr);
    //fflush(stdout);



    LPTSTR pipe_name = TEXT("\\\\.\\pipe\\testpipe");
    HANDLE pipe_h;

    while (1) {
        pipe_h = CreateFile(pipe_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, NULL);
        if (pipe_h != INVALID_HANDLE_VALUE) break;
    }

    //Initialize and allocate WSAPROTOCOL_INFO struct
    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;
    DWORD dwBufLen = 0;
    int nRet;
    DWORD dwErr;
    nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen);
    printf("The needed buffer size is %d bytes...\n", dwBufLen);
    if (nRet != SOCKET_ERROR)
        printf("WSAEnumProtocols() no SOCKET_ERROR! \n");
    else if ((dwErr = WSAGetLastError()) != WSAENOBUFS) {
        printf("Critical Problem Occurred\n");
        exit(EXIT_FAILURE);
        }
    else {
        printf("Error >expected< %d\nNow allocation space for the structure...\n", WSAGetLastError());
        lpProtocolBuf = (WSAPROTOCOL_INFO *)malloc(dwBufLen);
        if (nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen) == SOCKET_ERROR) {
            fprintf(stderr, "Couldn't allocate space for WSAPROTOCOL_INFO structure\n");
            exit(EXIT_FAILURE);
        }
    }
    fflush(stdout);

    DWORD cbRead;
    BOOL fSuccess;
    if (fSuccess = ReadFile(pipe_h, (void *) lpProtocolBuf, 1*dwBufLen, &cbRead, NULL) == FALSE)
        fprintf(stderr, "Couldn't Read from Pipe\n");
    printf("READ Success! read=%d (buf_size=%d)\n", cbRead, dwBufLen); // NB: Leggo la stessa qnt che ho scritto in server.c

    SOCKET sock_fd;
    sock_fd = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, lpProtocolBuf, 0, WSA_FLAG_OVERLAPPED);

    printf("SOCKET INFO: sock_fd=%d, address_family=%d, protocol=%d\n", sock_fd, lpProtocolBuf->iAddressFamily, lpProtocolBuf->iProtocol);
    //Family = 2 => AF_INET; PROTOCOL = 6 => TCP
    fflush(stdout);

    printf("Now trying to invoke routine\n");
    fflush(stdout);
    CloseHandle(pipe_h);
    process_routine((void *) &sock_fd);
#endif
    return 0;
}