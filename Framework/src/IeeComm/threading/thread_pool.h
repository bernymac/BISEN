#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <unistd.h>
#include <pthread.h>

#include "sgx_eid.h"
#include "sgx_urts.h"
#include "sgx_uae_service.h"
#include "Enclave_u.h"

#include "ocall.h"
#include "sgx_handler.h"
#include "thread_pool.h"

typedef struct thread_pool {
    pthread_t *threads;
    unsigned count;
} thread_pool;

thread_pool* init_thread_pool(sgx_enclave_id_t eid, unsigned thread_count);
void delete_thread_pool(thread_pool* pool);

#endif
