#ifndef __OUTSIDE_UTIL_H_
#define __OUTSIDE_UTIL_H_

#include <stdlib.h>
#include "types.h"

#ifndef _INC_FCNTL
#define _INC_FCNTL


#define O_RDONLY       0x0000
#define O_WRONLY       0x0001
#define O_RDWR         0x0002
#define O_APPEND       0x0008
#define O_CREAT        0x0100
#define O_TRUNC        0x0200
#define O_EXCL         0x0400
#define O_TEXT         0x4000
#define O_BINARY       0x8000
#define O_WTEXT        0x10000
#define O_U16TEXT      0x20000
#define O_U8TEXT       0x40000

#endif

namespace outside_util {
    // file i/o
    int open(const char *filename, int mode);
    ssize_t read(int file, void *buf, size_t len);
    ssize_t write(const int file, const void *buf, const size_t len);
    void close(int file);

    int open_socket(const char* addr, int port);
    void socket_send(int socket, const void* buff, size_t len);
    void socket_receive(int socket, void* buff, size_t len);

    // uee communication
    int open_uee_connection();
    void uee_process(const int socket, void **out, size_t *out_len, const void *in, const size_t in_len);
    void close_uee_connection(const int socket);

    // outside allocation
    void* outside_malloc(size_t length);
    void outside_free(void *pointer);

    // misc
    void printf(const char *fmt, ...);
    void exit(int status);
    untrusted_time curr_time();

    // DEBUG: for outside kmeans
    void set(size_t num_elems, float* buffer);
    float* get(const int pos);
    void print_bytes(const char* msg);
    void reset_bytes();

    // generic, implementable in extern
    int process(void **out, size_t *out_len, const void *in, const size_t in_len);
}
#endif
