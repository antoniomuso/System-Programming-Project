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

    char * msg= "GET / HTTP/1.1\r\n"
                "Host: 192.241.213.46:6880\r\n"
                "Upgrade-Insecure-Requests: 1\r\n"
                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_3) AppleWebKit/602.4.8 (KHTML, like Gecko) Version/10.0.3 Safari/602.4.8\r\n"
                "Accept-Language: en-us\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Connection: keep-alive\r\n\r\n";
    http_header http = parse_http_header_request(msg,strlen(msg));
    printf("User-Agent: %s\n Type: %s\n ", http.user_agent, http.type_req);

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

    run_server(opt, fopt);

    return 0;
}
