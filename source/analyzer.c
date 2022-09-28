#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "watchdog.h" 

void Analyzer(cut_t* cut_str){
    pthread_mutex_lock(&cut_str->common.cpu_data_mutex);
    pthread_mutex_lock(&cut_str->common.exit_flag_mutex);
    while(cut_str->common.new_data_flag == 0 && cut_str->common.exit_flag == 0){
        pthread_cond_wait(&cut_str->common.new_data_cond, &cut_str->common.cpu_data_mutex);
    }
    pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);           
    cut_str->cpu_data.curr_idle = cut_str->cpu_data.current_cpu_stats[3][cut_str->common.buffer_index] + cut_str->cpu_data.current_cpu_stats[4][cut_str->common.buffer_index];

    unsigned long long total_diff = cut_str->cpu_data.curr_total - cut_str->cpu_data.prev_total;
    unsigned long long idle_diff = cut_str->cpu_data.curr_idle - cut_str->cpu_data.prev_idle;

    cut_str->cpu_data.CPU_usage = ((float)(total_diff - idle_diff)/total_diff) * 100;

    if(cut_str->common.buffer_full == 1){
        int index = (cut_str->common.buffer_index+1)%BUFFER_SIZE;
        cut_str->cpu_data.prev_idle = cut_str->cpu_data.current_cpu_stats[3][index] + cut_str->cpu_data.current_cpu_stats[4][index];
        cut_str->cpu_data.prev_total = 0;
        for(int i = 0; i < 10; i++){
            cut_str->cpu_data.prev_total += cut_str->cpu_data.current_cpu_stats[i][index];
        }
    }else{
        cut_str->cpu_data.prev_idle = cut_str->cpu_data.curr_idle;
        cut_str->cpu_data.prev_total = cut_str->cpu_data.curr_total;
    }
    cut_str->common.new_data_flag = 0;
          
    pthread_mutex_unlock(&cut_str->common.cpu_data_mutex);
}

void* Analyzer_task(void* args){
    cut_t* cut_str = (cut_t*) args;
    while(1){
        usleep(100);
        Watchdog_Kick(cut_str);
        Analyzer(cut_str);
        pthread_mutex_lock(&cut_str->common.exit_flag_mutex);
        if(cut_str->common.exit_flag){
            pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
            break;
        }
        pthread_mutex_unlock(&cut_str->common.exit_flag_mutex);
    }
    return NULL;
}