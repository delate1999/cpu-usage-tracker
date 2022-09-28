#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "watchdog.h"
#include "printer.h"
#include "analyzer.h"
#include "reader.h"

cut_t cut;
pthread_t thr[4];

void Close(cut_t* cut_str){
    pthread_join(thr[0], NULL);
    pthread_join(thr[1], NULL);
    pthread_join(thr[2], NULL);
    pthread_join(thr[3], NULL);
    pthread_mutex_destroy(&cut_str->common.cpu_data_mutex);
    pthread_mutex_destroy(&cut_str->common.watchdog_mutex);
    pthread_mutex_destroy(&cut_str->common.exit_flag_mutex);
    pthread_cond_destroy(&cut_str->common.new_data_cond);
    exit(0);
}

void CtrlCHandler(){
    pthread_mutex_lock(&cut.common.exit_flag_mutex);
    cut.common.exit_flag = 1;
    pthread_mutex_unlock(&cut.common.exit_flag_mutex);
    sleep(1);
    printf("\nExiting...\n");
    Close(&cut);
}

int main(){
   memset(&cut, 0, sizeof(cut));
   pthread_mutex_init(&cut.common.cpu_data_mutex, NULL);
   pthread_mutex_init(&cut.common.watchdog_mutex, NULL);
   pthread_mutex_init(&cut.common.exit_flag_mutex,NULL);
   pthread_cond_init(&cut.common.new_data_cond, NULL);

   pthread_create(&thr[1], NULL, Reader_task, (void*)&cut);
   pthread_create(&thr[2], NULL, Printer_task, (void*)&cut);
   pthread_create(&thr[0], NULL, Analyzer_task, (void*)&cut);
   pthread_create(&thr[3], NULL, Watchdog_task, (void*)&cut);
   signal(SIGINT, CtrlCHandler);

    while(1){
        sleep(1);
    }

    return 0;
}

