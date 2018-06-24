//
// Created by anotoniomusolino on 14/06/18.
//

#ifndef SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#define SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
#include "command_parser.h"

int flag_restart;
int child_terminate;
void set_signal_handler(void *arr_proc, int type_size, int arr_len, int mod);
void set_child_handler();
void set_thread_event();


#endif //SYSTEM_PROGRAMMING_PROJECT_SIGNALS_H
