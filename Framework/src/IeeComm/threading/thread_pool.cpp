#include "thread_pool.h"

void* enter_enclave_thread(void* args) {
    sgx_enclave_id_t eid = *((sgx_enclave_id_t*)args);
    free(args);

    sgx_status_t status = ecall_thread_enter(eid);
    if (status != SGX_SUCCESS) {
        print_error_message(status);
        exit(-1);
    }

    return NULL;
}

thread_pool* init_thread_pool(sgx_enclave_id_t eid, unsigned thread_count) {
    thread_pool* pool = (thread_pool*)malloc(sizeof(thread_pool));
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    pool->count = thread_count;

    for (unsigned i = 0; i < thread_count; ++i) {
        void* arg = malloc(sizeof(sgx_enclave_id_t));
        memcpy(arg, &eid, sizeof(sgx_enclave_id_t));

        pthread_create(&(pool->threads[i]), NULL, enter_enclave_thread, arg);
    }

    return pool;
}

void delete_thread_pool(thread_pool* pool) {
    for (unsigned i = 0; i < pool->count; ++i)
        pthread_join(pool->threads[i], 0);

    free(pool->threads);
    free(pool);
}
