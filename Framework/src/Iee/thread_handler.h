#ifndef __THREAD_HANDLER_H
#define __THREAD_HANDLER_H

#include <stdlib.h>
#include <mutex>
#include <condition_variable>

#include "trusted_util.h"

typedef struct thread_data {
    std::mutex* lock;
    std::condition_variable* cond_var;
    int ready;
    int done;
    void* task_args;
    void* (*task)(void*);
} thread_data;

void thread_handler_init(unsigned nr_threads);
thread_data* thread_handler_add();

#endif
