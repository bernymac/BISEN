#include "trusted_util.h"

#include "sgx_tprotected_fs.h"

double trusted_util::time_elapsed_ms(untrusted_time start, untrusted_time end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return (double)micros_used / 1000;
}

// SGX_FILE*
void* trusted_util::open_secure(const char* name, const char* mode) {
    return sgx_fopen_auto_key(name, mode);
}

size_t trusted_util::write_secure(const void* ptr, size_t size, size_t count, void* stream) {
    return sgx_fwrite(ptr, size, count, (SGX_FILE*)stream);
}

size_t trusted_util::read_secure(void* ptr, size_t size, size_t count, void* stream) {
    return sgx_fread(ptr, size, count, (SGX_FILE*)stream);
}

void trusted_util::close_secure(void* stream) {
    sgx_fclose((SGX_FILE*)stream);
}
