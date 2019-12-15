#include "thread_handler.h"
#include "outside_util.h"

// thread_handler data
thread_data* threads = NULL;
unsigned entered_threads;

// task data
unsigned nr_tasks;

// main thread data
int has_main_thread;
void* main_task_args;
void* (*main_task)(void*);

void thread_handler_init(unsigned nr_threads) {
    threads = (thread_data*)malloc(sizeof(thread_data) * nr_threads);
    entered_threads = 0;
    nr_tasks = 0;

    has_main_thread = 0;

    for (unsigned i = 0; i < nr_threads; ++i) {
        threads[i].lock = new std::mutex();
        threads[i].cond_var = new std::condition_variable();
        threads[i].ready = 0;
        threads[i].done = 0;
        threads[i].task_args = NULL;
    }
}

thread_data* thread_handler_add() {
    return threads + entered_threads++;
}

const unsigned trusted_util::thread_get_count(){
    return entered_threads + 1; // account for the main thread
}

// task related
int trusted_util::thread_add_work(void* (*task)(void*), void* args) {
    if(!has_main_thread) {
        main_task = task;
        main_task_args = args;
        has_main_thread = 1;
        return 0;
    }

    if(nr_tasks < entered_threads) {
        threads[nr_tasks].task = task;
        threads[nr_tasks].task_args = args;
        nr_tasks++;
        return 0;
    }

    return 1;
}

void trusted_util::thread_do_work() {
    //outside_util::printf("nr tasks %u\n", nr_tasks);
    // make threads work
    for (unsigned i = 0; i < nr_tasks; ++i) {
        thread_data* t = &threads[i];
        {
            std::lock_guard <std::mutex> lk(*t->lock);
            threads[i].ready = 1;
        }
        t->cond_var->notify_one();
    }

    // main thread's work
    main_task(main_task_args);

    // wait for completion of other threads
    for (unsigned i = 0; i < nr_tasks; ++i) {
        thread_data *t = &threads[i];
        {
            std::unique_lock <std::mutex> lk(*t->lock);
            threads[i].cond_var->wait(lk, [&t] { return t->done; });
        }

        // reset
        threads[i].done = 0;
        threads[i].task_args = NULL;
    }

    // reset for next task
    nr_tasks = 0;
    main_task = NULL;
    main_task_args = NULL;
    has_main_thread = 0;
}

