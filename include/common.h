#include <pthread.h>

#define BUFFER_SIZE              8
#define MINIMAL_TASK_ELAPSED     20

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

typedef struct{
    cpu_stats_t cpu_data;
    runtime_data_t common;
}cut_t;

void Close(cut_t* cut_str);