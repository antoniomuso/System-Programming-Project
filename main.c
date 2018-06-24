#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEBUG 1
#include "command_parser.h"
#include "server.h"



#ifdef __unix__

#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#elif _WIN32


#endif


int main(int argc, char *argv[]) {

    command_arc comm[] = { {"-n_proc","int"},{"-port", "int"},{"-server_ip", "str"}, {"-mode","str"}};
    options opt = options_parse(argc, argv, comm, 4);
    //get_command_value((char *) comm, options);

    command_arc confs[] = { {"n_proc", "int"}, {"port", "int"}, {"server_ip", "str"}, {"mode", "str"} };
    options fopt = parse_file("config.txt", confs, 4);

    if (is_options_error(fopt)) {
        exit(EXIT_FAILURE);
    }


#ifdef __unix__
    printf("Unix\n");
    fflush(stdout);
#if DEBUG != 1
    // Daemonization
    pid_t pid;

    pid = fork();
    if (pid < 0)
        return(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    // The daemon is created
#endif



#endif

#if _WIN32
    printf("Windows\n");
#endif

    // If the server returns 0, the configuration file must be reloaded in order to re-launch the server.
    while (run_server(opt, fopt) == 0) {

        fopt = parse_file("config.txt", confs, 4);
        if (is_options_error(fopt)) {
            exit(EXIT_FAILURE);
        }
        opt.commands = NULL;
    }

    fprintf(stderr,"An error occurred while trying to reload the server.");
    return 1;
}
