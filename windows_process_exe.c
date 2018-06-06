#include "command_parser.h"
#include "server.h"


#include <stdio.h>
#include <stdlib.h>

#ifdef  _WIN32
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    int sock_fd = atoi(argv[0]);
    int *sock_ptr;
    *sock_ptr = sock_fd;
    process_routine((void *) sock_ptr);
#endif
    return 0;
}