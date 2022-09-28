#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "watchdog.h" 

void Printer(cut_t* cut_str){
    printf("\rTotal CPU usage: %6.2f%%", cut_str->cpu_data.CPU_usage);
    fflush(stdout);  
}

void* Printer_task(void* args){
    cut_t* cut_str = (cut_t*) args;
    while(1){
        Watchdog_Kick(cut_str);
        sleep(1);
        Printer(cut_str);
        pthread_mutex_lock(&cut_str->common.exit_flag_mutex);
        if(cut_str->common.exit_flag){
            pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
            break;
        }
        pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
    }
    return NULL;
}