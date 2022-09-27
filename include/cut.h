#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define BUFFER_SIZE              8
#define MINIMAL_TASK_ELAPSED     20