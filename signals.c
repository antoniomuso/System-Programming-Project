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

void * arr_process = NULL; // array of processes/threads
int len = 0;
int mode = 0;

int flag_restart = 0;


#ifdef _WIN32
const char * event_name = "threadevent";

void initialize_windows_event () {
    SECURITY_ATTRIBUTES sattr;

    sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sattr.bInheritHandle = TRUE;
    sattr.lpSecurityDescriptor = NULL;

    if (CreateEvent(&sattr, TRUE, FALSE, event_name) == NULL) {
        fprintf(stderr, "Couldn't create\\open event \"%s\" (%d)", event_name, GetLastError());
        ExitThread(1);
    }
    //printf("Event set\n");
    fflush(stdout);
}
#endif

int is_reloading(void * args) {
#ifdef __unix__
    if (flag_restart == 1) {
        return 1;
    }
#elif _WIN32
    HANDLE event = *((HANDLE *)(args));
    DWORD out = WaitForSingleObject(event, 0);
    if (out == WAIT_OBJECT_0) {
        return 1;
    }
#endif
    return 0;
}

void reset_events() {

    flag_restart = 0;

#ifdef _WIN32
    HANDLE eventp;
    if ((eventp = OpenEvent(EVENT_ALL_ACCESS, FALSE, event_name)) == NULL) {
        fprintf(stderr, "Failed Opening Event\n");
        exit(EXIT_FAILURE);
    }
    if (!ResetEvent(eventp)) {
        fprintf(stderr, "Failed Resetting Event\n");
        exit(EXIT_FAILURE);
    }
    CloseHandle(eventp);
#endif

}

int infanticide(void *children_array, int len, int mode, int exit_code) {
    /**
     * Mode: 0 = MT, 1 = MP.
     */
#ifdef _WIN32
    HANDLE *array = (HANDLE *) children_array;

    char *event_name = "threadevent";
    HANDLE event;
    if ((event = OpenEvent(EVENT_ALL_ACCESS, FALSE, event_name)) == NULL) {
        fprintf(stderr, "Failed Opening Event \"%s\" (%d)\n", event_name, GetLastError());
        return 1;
    }
    if(SetEvent(event) == FALSE) {
        fprintf(stderr, "SetEvent Failed %d\n", GetLastError());
        CloseHandle(event);
        return 1;
    }
    //printf("Event set\n");
    //fflush(stdout);
    // Wait for threads/processes to exit
    if((WaitForMultipleObjects(len, children_array, TRUE, INFINITE)) == WAIT_FAILED) {
        fprintf(stderr,"WaitForMultipleObjects error.\n");
        return 1;
    }
    CloseHandle(event);

#elif __unix__
    if (mode == 0) {
        pthread_t *tids = (pthread_t *) children_array;
        for (int i = 0; i < len; i++) {
            if (pthread_kill(tids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return 1;
            }
            // Wait for threads to exit
            if (pthread_join(tids[i], NULL) != 0) {
                fprintf(stderr,"pthread_join error\n");
                return 1;
            }
        }
        return len;

    } else if (mode == 1) {
        int *pids = (int *) children_array;

        for (int i = 0; i < len; i++) {
            if (kill(pids[i], exit_code) == -1) {
                fprintf(stderr,"%s\n", strerror(errno));
                return 1;
            }
            int status = 0;
            // Wait for processes to exit
            if (waitpid(pids[i],&status,0) < 0  && errno != ECHILD) {
                fprintf(stderr,"waitpid error\n");
                return 1;
            }

        }
    }

#endif
    return 0;
}

#ifdef __unix__
void child_handler (int signal) {
    if (signal == SIGUSR1) {
        flag_restart = 1;
    }
}
#endif

#ifdef __unix__
void set_child_handler () {
    struct sigaction sa;
    //printf("proc id: %d\n", getpid());

    // Setup the signal handler
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
}
#elif _WIN32
HANDLE set_child_handler() {
    HANDLE event_h;
    if ((event_h = OpenEvent(EVENT_ALL_ACCESS, FALSE, event_name)) == NULL) {
        fprintf(stderr, "OpenEvent failed.\n");
        exit(EXIT_FAILURE);
    }
    return event_h;
}
#endif


#ifdef __unix__
void handle_signal(int signal) {
    if (infanticide(arr_process, len, mode, SIGUSR1) == 1) {
        fprintf(stderr, "An error occurred while trying to kill a child process.");
        exit(EXIT_FAILURE);
    }
    free(arr_process);
    flag_restart = 1;
}


#elif _WIN32
BOOL ctrl_handler(DWORD ctrl_type) {
    printf("Intercepted: %d\n", ctrl_type);
    fflush(stdout);
    if (ctrl_type == CTRL_BREAK_EVENT) { // CTRL+Break triggers the operation
        printf("Entering handler for %d...\n", ctrl_type);
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

    // Setup the signal handler
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
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrl_handler, FALSE);

    if( !(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrl_handler, TRUE )) ) {
        fprintf(stderr,"Error: couldn't set control handler"); // Should not happen
        exit(EXIT_FAILURE);
    }
#endif
}

#ifdef __unix__
void signal_terminate (int sig) {
    handle_signal(sig);
    exit(0);
}

int set_signal_server_exit () {
    struct sigaction sa;
    //printf("proc id: %d\n", getpid());

    // Setup the signal handler
    sa.sa_handler = &signal_terminate;

    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    if (sigfillset(&sa.sa_mask) == -1) {
        fprintf(stderr, "sigfillset error in set_signal_handler\n");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr,"Error: cannot handle SIGHUP"); // Should not happen
        exit(EXIT_FAILURE);
    }

}
#endif