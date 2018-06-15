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
    // TODO: Qui ci va un Parser per le opzioni.

    command_arc comm[] = { {"-n_proc","int"},{"-port", "int"},{"-server_ip", "str"}, {"-mode","str"}};
    options opt = options_parse(argc, argv, comm, 4); // Il puntatore dentro options va liberato.
    //get_command_value((char *) comm, options);

    command_arc confs[] = { {"n_proc", "int"}, {"port", "int"}, {"server_ip", "str"}, {"mode", "str"} };
    options fopt = parse_file("config.txt", confs, 4);

    options *p_fopt = &fopt;
    options *p_opt = &opt;


#ifdef __unix__
    printf("Unix\n");
    fflush(stdout);
#if DEBUG != 1
    // Daemon fork
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        return(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Qui il demone è nato.
#endif



#endif

#if _WIN32
    printf("Windows\n");
#endif

    while (run_server(p_opt, p_fopt) == 0) {
        fopt = parse_file("config.txt", confs, 4);

        p_fopt = &fopt;
        p_opt = NULL;
    }

    return 1;
}
