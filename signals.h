//
// Created by anotoniomusolino on 14/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#define SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#include "command_parser.h"

int flag_restart;

void set_signal_handler(void *arr_proc, int type_size, int arr_len, int mod);
void set_child_handler(void * event);
void set_thread_event();
void reset_events ();
int is_reloading (void * event);
void initialize_windows_event ();


#endif //SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
