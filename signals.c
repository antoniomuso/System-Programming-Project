//
// Created by anotoniomusolino on 14/06/18.
//

#include "signals.h"
#include <stdlib.h>
#include <stdio.h>
#include "server.h"
#include <errno.h>
#include <string.h>
#include "command_parser.h"

#ifdef __unix__
#include <pthread.h>
#include <signal.h>
#elif _WIN32
#include <windows.h>
#endif

void * arr_process; // array of process
int len = 0;
int mode = 0;


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

void set_signal_handler(void *arr_proc, int arr_len, int mod);





#ifdef __unix__
void handle_signal(int signal) {
    infanticide(arr_process, len, mode, SIGKILL);
    command_arc confs[] = { {"n_proc", "int"}, {"port", "int"}, {"server_ip", "str"}, {"mode", "str"} };
    options fopt = parse_file("config.txt", confs, 4);
    run_server(NULL, &fopt);
}

void set_signal_handler(void *arr_proc, int arr_len, int mod) {
    arr_process = arr_proc;
    mode = mod;
    len = arr_len;

    struct sigaction sa;

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