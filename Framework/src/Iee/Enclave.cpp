#include "Enclave.h"

#include <stdint.h>
#include <string.h>
#include <mutex>
#include <condition_variable>

#include "outside_util.h"
#include "trusted_crypto.h"
#include "thread_handler.h"
#include "extern_lib.h"

void ecall_init_enclave(unsigned nr_threads) {
    thread_handler_init(nr_threads);

    // let extern lib do its own init
    extern_lib::init();
}

void ecall_thread_enter() {
    // register thread as entering
    thread_data* my_data = thread_handler_add();

    while (1) {
        // wait until we have work
        std::unique_lock<std::mutex> lock(*my_data->lock);
        my_data->cond_var->wait(lock, [&my_data]{return my_data->ready;});

        // extern lib's own handling
        void* res = my_data->task(my_data->task_args); // TODO pass res to thread_handler via my_data?

        my_data->ready = 0;
        my_data->done = 1;
        lock.unlock();
        my_data->cond_var->notify_one();
    }
}
/*
void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    uint8_t key[crypto_secretbox_KEYBYTES];
    memset(key, 0x00, crypto_secretbox_KEYBYTES);

    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);

    // decrypt input
    //uint8_t* in_unenc = (uint8_t*)malloc(in_len - SODIUM_EXPBYTES);
    //tcrypto::sodium_decrypt(in_unenc, (uint8_t*)in, in_len, nonce, key);

    // prepare unencrypted output
    //size_t out_unenc_len = 0;
    //uint8_t* out_unenc = NULL;
    extern_lib::process_message((uint8_t**)out, out_len, (const uint8_t *) in, in_len);

    // encrypt output
    //*out_len = out_unenc_len + SODIUM_EXPBYTES;
    //*out = malloc(*out_len);
    //tcrypto::sodium_encrypt((uint8_t*)*out, out_unenc, out_unenc_len, nonce, key);

    // cleanup
    //free(in_unenc);
    //free(out_unenc);
}
*/
