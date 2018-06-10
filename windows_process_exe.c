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

    printf("Initialising Winsock...\n");
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
    HANDLE hPipe;

    while (1) {
        //printf("Entered Loop\n");
        //fflush(stdout);
        hPipe = CreateFile(
                pipe_name,
                GENERIC_READ,
                0,
                0,
                OPEN_EXISTING,
                0,
               NULL);
        if (hPipe != INVALID_HANDLE_VALUE) break;
        //printf("%d", GetLastError());
    }

    BOOL fSuccess;

    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;
    DWORD dwBufLen = 0;
    int nRet;
    DWORD dwErr;
    nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen);
    printf("The buffer size is %d bytes...\n", dwBufLen);
    if (nRet != SOCKET_ERROR)
        printf("WSAEnumProtocols() no SOCKET_ERROR! \n");
    else if ((dwErr = WSAGetLastError()) != WSAENOBUFS)
        printf("Big problemos\n");
    else {
        printf("Error >expected< %d\n", WSAGetLastError());
        lpProtocolBuf = (WSAPROTOCOL_INFO *)malloc(dwBufLen);
        nRet = WSAEnumProtocols(NULL, lpProtocolBuf, &dwBufLen);
        if (nRet == SOCKET_ERROR)
            printf("HUGE PROBLEM\n");
    }

    fflush(stdout);

    DWORD cbRead;

    fSuccess = ReadFile(
            hPipe,
            (void *) lpProtocolBuf,
            1*dwBufLen,
            &cbRead,
            NULL);


    if (fSuccess) printf("READ Success! read=%d (size=%d)\n", cbRead, dwBufLen); // NB: Leggo la stessa qnt che ho scritto in server.c
    //printf("%s\n", buff);
    SOCKET sock_fd;
    sock_fd = WSASocket(
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            lpProtocolBuf,
            0,
            WSA_FLAG_OVERLAPPED);

    printf("HERE\n");
    printf("sock_res = %d\n", sock_fd);


    fflush(stdout);

    /*
     * Problema da qui
     */
    //int *sock_tr;
    //*sock_tr = (int) sock_fd;

    printf("Now trying to invoke routine\n");
    fflush(stdout);
    CloseHandle(hPipe);
    process_routine((void *) &sock_fd);
#endif
    return 0;
}