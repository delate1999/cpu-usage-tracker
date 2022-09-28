#include <stdio.h>
#include <stdlib.h>
#include "cut.h"

typedef struct{
    float CPU_usage;
    unsigned long long current_cpu_stats[10][BUFFER_SIZE];
    unsigned long long curr_total;
    unsigned long long curr_idle;
    unsigned long long prev_idle;
    unsigned long long prev_total;
}cpu_stats_t;

typedef struct{
    clock_t  last_watchdog_kick_time;
    int buffer_index;
    int buffer_full;
    pthread_mutex_t cpu_data_mutex;
    pthread_mutex_t watchdog_mutex;
    pthread_mutex_t exit_flag_mutex;
    pthread_cond_t new_data_cond;
    int new_data_flag;
    int exit_flag;
}runtime_data_t;

cpu_stats_t cpu_data;
runtime_data_t common;
pthread_t thr[4];

void Close(){
    pthread_join(thr[0], NULL);
    pthread_join(thr[1], NULL);
    pthread_join(thr[2], NULL);
    pthread_join(thr[3], NULL);
    pthread_mutex_destroy(&common.cpu_data_mutex);
    pthread_mutex_destroy(&common.watchdog_mutex);
    pthread_mutex_destroy(&common.exit_flag_mutex);
    pthread_cond_destroy(&common.new_data_cond);
    exit(0);
}

void Watchdog_Kick(){
    pthread_mutex_lock(&common.watchdog_mutex);
    common.last_watchdog_kick_time = clock();
    pthread_mutex_unlock(&common.watchdog_mutex);
}

void Printer(void){
    printf("\rTotal CPU usage: %6.2f%%", cpu_data.CPU_usage);
    fflush(stdout);  
}

void* Printer_task(void* arg_unused){
    if(arg_unused != NULL){
        Close();
    }else{
        while(1){
            Watchdog_Kick();
            sleep(1);
            Printer();
            pthread_mutex_lock(&common.exit_flag_mutex);
            if(common.exit_flag){
                pthread_mutex_unlock(&common.exit_flag_mutex);
                break;
            }
            pthread_mutex_unlock(&common.exit_flag_mutex);
        }
    }
    return NULL;
}

void Reader(){
    FILE *fp;
    char string_buff[255];
    fp= fopen("/proc/stat", "r");
    if(fp == NULL){
        printf("\n/proc/stat opening error!");
        Close();
    }else{
        unsigned long long cpu_data_buffer[10];
        unsigned long long cpu_data_buffer_sum = 0;
        fgets(string_buff, sizeof(string_buff), fp);
        sscanf(string_buff, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &cpu_data_buffer[0], &cpu_data_buffer[1], &cpu_data_buffer[2], &cpu_data_buffer[3], &cpu_data_buffer[4],
                                &cpu_data_buffer[5], &cpu_data_buffer[6], &cpu_data_buffer[7], &cpu_data_buffer[8], &cpu_data_buffer[9]);
        for(int i = 0; i < 10; i++){
            cpu_data_buffer_sum += cpu_data_buffer[i];
        }
        if((cpu_data_buffer_sum - cpu_data.curr_total) > MINIMAL_TASK_ELAPSED){
            pthread_mutex_lock(&common.cpu_data_mutex);
            cpu_data.curr_total = cpu_data_buffer_sum;
            if(common.buffer_index == (BUFFER_SIZE-1)){
                common.buffer_full = 1;
            }
            common.buffer_index = (common.buffer_index+1)%BUFFER_SIZE;
            for(int i = 0; i < 10; i++){
                cpu_data.current_cpu_stats[i][common.buffer_index] = cpu_data_buffer[i];
            }
            common.new_data_flag = 1;
            pthread_cond_signal(&common.new_data_cond);
            pthread_mutex_unlock(&common.cpu_data_mutex);
        }else{
            common.new_data_flag = 0;
        }
        fclose(fp);
    }
}

void* Reader_task(void* arg_unused){
    if(arg_unused != NULL){
        Close();
    }else{
        while(!common.exit_flag){
            usleep(100);
            Watchdog_Kick();
            Reader();
        }
    }
    return NULL;    
}

void Analyzer(void){
    pthread_mutex_lock(&common.cpu_data_mutex);
    pthread_mutex_lock(&common.exit_flag_mutex);
    while(common.new_data_flag == 0 && common.exit_flag == 0){
        pthread_cond_wait(&common.new_data_cond, &common.cpu_data_mutex);
    }
    pthread_mutex_unlock(&common.exit_flag_mutex);           
    cpu_data.curr_idle = cpu_data.current_cpu_stats[3][common.buffer_index] + cpu_data.current_cpu_stats[4][common.buffer_index];

    unsigned long long total_diff = cpu_data.curr_total - cpu_data.prev_total;
    unsigned long long idle_diff = cpu_data.curr_idle - cpu_data.prev_idle;

    cpu_data.CPU_usage = ((float)(total_diff - idle_diff)/total_diff) * 100;

    if(common.buffer_full == 1){
        int index = (common.buffer_index+1)%BUFFER_SIZE;
        cpu_data.prev_idle = cpu_data.current_cpu_stats[3][index] + cpu_data.current_cpu_stats[4][index];
        cpu_data.prev_total = 0;
        for(int i = 0; i < 10; i++){
            cpu_data.prev_total += cpu_data.current_cpu_stats[i][index];
        }
    }else{
        cpu_data.prev_idle = cpu_data.curr_idle;
        cpu_data.prev_total = cpu_data.curr_total;
    }
    common.new_data_flag = 0;
          
    pthread_mutex_unlock(&common.cpu_data_mutex);
}

void* Analyzer_task(void* arg_unused){
    if(arg_unused != NULL){
        Close();
    }else{
        while(1){
            usleep(100);
            Watchdog_Kick();
            Analyzer();
            pthread_mutex_lock(&common.exit_flag_mutex);
            if(common.exit_flag){
                pthread_mutex_unlock(&common.exit_flag_mutex);
                break;
            }
            pthread_mutex_unlock(&common.exit_flag_mutex);
        }
    }
    return NULL;
}

void CtrlCHandler(){
    pthread_mutex_lock(&common.exit_flag_mutex);
    common.exit_flag = 1;
    pthread_mutex_unlock(&common.exit_flag_mutex);
    sleep(1);
    printf("\nExiting...\n");
    Close();
}

void Watchdog(void){
    clock_t current_watchdog_kick_time = clock();
    double diff = ((double)(current_watchdog_kick_time - common.last_watchdog_kick_time))/CLOCKS_PER_SEC;
    if(diff > 2.0){
        printf("\nWatchdog error!");
        Close();
    }
}

void* Watchdog_task(void* arg_unused){
    if(arg_unused != NULL){
        Close();
    }else{
        while(1){
            usleep(100);
            Watchdog();
            pthread_mutex_lock(&common.exit_flag_mutex);
            if(common.exit_flag){
                pthread_mutex_unlock(&common.exit_flag_mutex);
                break;
            }
            pthread_mutex_unlock(&common.exit_flag_mutex);
        }
    }
    return NULL;    
}

int main(){
   memset(&cpu_data, 0, sizeof(cpu_data));
   memset(&common, 0, sizeof(common));
   pthread_mutex_init(&common.cpu_data_mutex, NULL);
   pthread_mutex_init(&common.watchdog_mutex, NULL);
   pthread_mutex_init(&common.exit_flag_mutex,NULL);
   pthread_cond_init(&common.new_data_cond, NULL);

   pthread_create(&thr[1], NULL, Reader_task, NULL);
   pthread_create(&thr[2], NULL, Printer_task, NULL);
   pthread_create(&thr[0], NULL, Analyzer_task, NULL);
   pthread_create(&thr[3], NULL, Watchdog_task, NULL);
   signal(SIGINT, CtrlCHandler);

    while(1){
        sleep(1);
    }

    return 0;
}

