#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init_enclave(unsigned nr_threads);
void ecall_thread_enter();
//void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
