//
// Created by anotoniomusolino on 14/06/18.
//

#include "signals.h"
#include <stdlib.h>
#include <stdio.h>
#include "server.h"
#include "command_parser.h"

#ifdef __unix__
#include <signal.h>
#elif _WIN32
#endif

void * arr_process; // array of process


int infanticide(void *children_array, int len, int mode, int exit_code) {
    /**
     * Mode: 0 = MT, 1 = MP.
     */
    int i = 0;
#ifdef _WIN32
    HANDLE *array = (HANDLE *) children_array;
    if (mode == 0) {
        for (i = 0; i < len; i++) {
            if(!TerminateThread(array[i], exit_code))
                return i;
        }
    } else if (mode == 1) {
        for (i = 0; i < len; i++) {
            if(!TerminateProcess(array[i], exit_code))
                return i;
        }
    }
#elif __unix__

    if (mode == 0) {
        pthread_t *tids = (pthread_t *) children_array;

        for (i = 0; i < len; i++) {
            int err = 0;
            if (err = pthread_cancel(tids[i]) != 0) {
                fprintf(stderr,"%s\n", strerror(err));
                return i;
            }
        }
    } else if (mode == 1) {
        int *pids = (int *) children_array;

        for (i = 0; i < len; i++) {
            if (kill(pids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return i;
            }
        }
    }

#endif
    return i;
}

void set_signal_handler(void *arr_proc, int arr_len);





#ifdef __unix__
void handle_signal(int signal) {
    infanticide(arr_process);
    command_arc confs[] = { {"n_proc", "int"}, {"port", "int"}, {"server_ip", "str"}, {"mode", "str"} };
    options fopt = parse_file("config.txt", confs, 4);
    run_server(NULL, &fopt);
}

void set_signal_handler(void *arr_proc, int arr_len) {
    arr_process = arr_proc;
    struct sigaction sa;
    // Print pid, so that we can send signals from other shells
    printf("My pid is: %d\n", getpid());

    // Setup the sighub handler
    sa.sa_handler = &handle_signal;

    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr,"Error: cannot handle SIGHUP"); // Should not happen
        exit(EXIT_FAILURE);
    }

}
#elif _WIN32


#endif