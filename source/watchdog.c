#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "watchdog.h"

void Watchdog_Kick(cut_t* cut_str){
    pthread_mutex_lock(&cut_str->common.watchdog_mutex);
    cut_str->common.last_watchdog_kick_time = clock();
    pthread_mutex_unlock(&cut_str->common.watchdog_mutex);
}

void Watchdog(cut_t* cut_str){
    clock_t current_watchdog_kick_time = clock();
    double diff = ((double)(current_watchdog_kick_time - cut_str->common.last_watchdog_kick_time))/CLOCKS_PER_SEC;
    if(diff > 2.0){
        printf("\nWatchdog error!");
        Close(cut_str);
    }
}

void* Watchdog_task(void* args){
    cut_t* cut_str = (cut_t*) args;
    while(1){
        usleep(100);
        Watchdog(cut_str);
        pthread_mutex_lock(&cut_str->common.exit_flag_mutex);
        if(cut_str->common.exit_flag){
            pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
            break;
        }
        pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
    }
    return NULL;    
}