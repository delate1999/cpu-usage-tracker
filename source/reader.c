#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "watchdog.h" 

void Reader(cut_t* cut_str){
    FILE *fp;
    char string_buff[255];
    fp= fopen("/proc/stat", "r");
    if(fp == NULL){
        printf("\n/proc/stat opening error!");
        Close(cut_str);
    }else{
        unsigned long long cpu_data_buffer[10];
        unsigned long long cpu_data_buffer_sum = 0;
        fgets(string_buff, sizeof(string_buff), fp);
        sscanf(string_buff, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &cpu_data_buffer[0], &cpu_data_buffer[1], &cpu_data_buffer[2], &cpu_data_buffer[3], &cpu_data_buffer[4],
                                &cpu_data_buffer[5], &cpu_data_buffer[6], &cpu_data_buffer[7], &cpu_data_buffer[8], &cpu_data_buffer[9]);
        for(int i = 0; i < 10; i++){
            cpu_data_buffer_sum += cpu_data_buffer[i];
        }
        if((cpu_data_buffer_sum - cut_str->cpu_data.curr_total) > MINIMAL_TASK_ELAPSED){
            pthread_mutex_lock(&cut_str->common.cpu_data_mutex);
            cut_str->cpu_data.curr_total = cpu_data_buffer_sum;
            if(cut_str->common.buffer_index == (BUFFER_SIZE-1)){
                cut_str->common.buffer_full = 1;
            }
            cut_str->common.buffer_index = (cut_str->common.buffer_index+1)%BUFFER_SIZE;
            for(int i = 0; i < 10; i++){
                cut_str->cpu_data.current_cpu_stats[i][cut_str->common.buffer_index] = cpu_data_buffer[i];
            }
            cut_str->common.new_data_flag = 1;
            pthread_cond_signal(&cut_str->common.new_data_cond);
            pthread_mutex_unlock(&cut_str->common.cpu_data_mutex);
        }else{
            cut_str->common.new_data_flag = 0;
        }
        fclose(fp);
    }
}

void* Reader_task(void* args){
    cut_t* cut_str = (cut_t*) args;
    while(!cut_str->common.exit_flag){
        usleep(100);
        Watchdog_Kick(cut_str);
        Reader(cut_str);
    }
    return NULL;    
}