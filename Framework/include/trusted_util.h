#ifndef __TRUSTED_UTIL_H
#define __TRUSTED_UTIL_H

#include <stdlib.h>
#include "types.h"

namespace trusted_util {
    // threading
    const unsigned thread_get_count();
    int thread_add_work(void* (*task)(void*), void* args);
    void thread_do_work();

    // time
    double time_elapsed_ms(untrusted_time start, untrusted_time end);

    // secure file i/o
    void* open_secure(const char* name, const char* mode);
    size_t write_secure(const void* ptr, size_t size, size_t count, void* stream);
    size_t read_secure(void* ptr, size_t size, size_t count, void* stream);
    void close_secure(void* stream);
}

#endif
