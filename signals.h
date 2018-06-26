#ifndef SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#define SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#include "command_parser.h"

int flag_restart;

void set_signal_handler(void *arr_proc, int type_size, int arr_len, int mod);
#ifdef __unix__
void set_child_handler();
int set_signal_server_exit ();
#elif _WIN32
#include <windows.h>
HANDLE set_child_handler();
#endif
void set_thread_event();
void reset_events ();
int is_reloading (void * event);
void initialize_windows_event ();


#endif //SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
