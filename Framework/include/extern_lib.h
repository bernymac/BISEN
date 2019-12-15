#ifndef __EXTERN_LIB_H_
#define __EXTERN_LIB_H_

#include <stdlib.h>
#include <stdint.h>

// external lib should implement these
namespace extern_lib {
    // called before threading is initialised
    void init();

    // process a message from the client
    // *out should be allocated inside, and is freed by the framework
    void process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len);
}

namespace extern_lib_ut {
    // process a generic ocall, done from the enclave
    // *out should be allocated inside, and is freed by the framework
    void process_message(void** out, size_t* out_len, const void* in, const size_t in_len);
}

#endif
