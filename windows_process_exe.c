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

SOCKET ConvertProcessSocket(SOCKET oldsocket, DWORD source_pid) {
    HANDLE source_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, source_pid);
    printf("%d\n", source_handle);

    HANDLE newhandle;
    if(DuplicateHandle(source_handle, (HANDLE)oldsocket, GetCurrentProcess(), &newhandle, 0, TRUE, DUPLICATE_SAME_ACCESS)) printf("success\n");
    //CloseHandle(source_handle);
    fflush(stdout);
    return (SOCKET)newhandle;
}
#endif

int main(int argc, char* argv[]) {
    printf("ciao");
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
    int sock_fd = atoi(argv[0]);
    int parent_id = atoi(argv[1]);

    printf("%d %d\n", sock_fd, parent_id);
    fflush(stdout);

    int *sock_ptr;
    *sock_ptr = ConvertProcessSocket(sock_fd, parent_id);
    printf("%d\n", *sock_ptr);
    fflush(stdout);

    process_routine((void *) sock_ptr);
#endif
    return 0;
}