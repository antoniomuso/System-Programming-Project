//
// Created by anotoniomusolino on 14/06/18.
//

#include "signals.h"
#include <stdlib.h>
#include <stdio.h>
#include "server.h"
#include <errno.h>
#include <string.h>

#ifdef __unix__

#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#elif _WIN32
#include <windows.h>
#endif

void * arr_process = NULL; // array of process
int len = 0;
int mode = 0;

int flag_restart = 0;
int child_terminate = 0;



int infanticide(void *children_array, int len, int mode, int exit_code) {
    /**
     * Mode: 0 = MT, 1 = MP.
     */
    int i = 0;
#ifdef _WIN32
    HANDLE *array = (HANDLE *) children_array;
    if (mode == 0) {
         child_terminate = 1;
         return len;
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
            if (pthread_kill(tids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return i;
            }
            int s = 1;
            wait(&s);
            printf("thread: %d\n", s);
            fflush(stdout);
        }
        return len;

    } else if (mode == 1) {
        int *pids = (int *) children_array;

        for (i = 0; i < len; i++) {
            printf("kill: %d\n",pids[i]);
            int status = 1;
            if (kill(pids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return i;
            }
            waitpid(pids[i], &status, 0);
        }
    }

#endif
    return i;
}

void set_signal_handler(void *arr_proc, int type_size, int arr_len, int mod);

void child_handler (int signal) {
    if (signal == SIGUSR1) {
        child_terminate = 1;
    }
}


void set_child_handler () {
#ifdef __unix__
    struct sigaction sa;
    //printf("proc id: %d\n", getpid());

    // Setup the sighub handler
    sa.sa_handler = &child_handler;

    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    if (sigfillset(&sa.sa_mask) == -1) {
        fprintf(stderr, "sigfillset error in set_signal_handler\n");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr,"Error: cannot handle SIGHUP"); // Should not happen
        exit(EXIT_FAILURE);
    }

#endif
}


#ifdef __unix__
void handle_signal(int signal) {
    infanticide(arr_process, len, mode, SIGUSR1);
    free(arr_process);
    flag_restart = 1;
}


#elif _WIN32
BOOL ctrl_handler(DWORD ctrl_type) {
    printf("intercepted %d \n", ctrl_type);
    fflush(stdout);
    if (ctrl_type == CTRL_BREAK_EVENT) { // CTRL+Break triggers the operation
        printf("entering handler for %d \n", ctrl_type);
        fflush(stdout);
        infanticide(arr_process, len, mode, 0);
        free(arr_process);
        flag_restart = 1;
        return TRUE;
    }
    return FALSE;
}

#endif

void set_signal_handler(void *arr_proc, int type_size, int arr_len, int mod) {
    arr_process = calloc(arr_len, type_size);
    if (arr_process == NULL) {
        fprintf(stderr, "Malloc error in set_signal_handler\n");
        exit(EXIT_FAILURE);
    }
    mode = mod;
    len = arr_len;

    memcpy(arr_process,arr_proc,arr_len*type_size);

#ifdef __unix__
    struct sigaction sa;
    //printf("proc id: %d\n", getpid());

    // Setup the sighub handler
    sa.sa_handler = &handle_signal;

    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    if (sigfillset(&sa.sa_mask) == -1) {
        fprintf(stderr, "sigfillset error in set_signal_handler\n");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr,"Error: cannot handle SIGHUP"); // Should not happen
        exit(EXIT_FAILURE);
    }

#elif _WIN32
    if( !(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrl_handler, TRUE )) ) {
        fprintf(stderr,"Error: couldn't set control handler"); // Should not happen
        exit(EXIT_FAILURE);
    }
#endif

}