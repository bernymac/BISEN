#include "ocall.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "definitions.h"
#include "extern_lib.h"
#include "untrusted_util.h"
#include "types.h"

/****************************************************** FILE I/O ******************************************************/
int ocall_open(const char* filename, int mode) {
    return open(filename, mode);
}

ssize_t ocall_read(int file, void *buf, size_t len) {
    return read(file, buf, len);
}

ssize_t ocall_write(const int file, const void *buf, const size_t len) {
    return write(file, buf, len);
}

void ocall_close(int file) {
    close(file);
}
/**************************************************** END FILE I/O ****************************************************/

/******************************************************** MISC ********************************************************/
void ocall_string_print(const char *str) {
    printf("%s", str);
}

void ocall_exit(int code) {
    exit(code);
}

untrusted_time ocall_curr_time() {
    timeval curr;
    gettimeofday(&curr, NULL);

    untrusted_time t;
    t.tv_sec = curr.tv_sec;
    t.tv_usec = curr.tv_usec;

    return t;
}
/****************************************************** END MISC ******************************************************/

int ocall_open_socket(const char* addr, int port) {
    return untrusted_util::socket_connect(addr, port);
}

void ocall_socket_send(int socket, const void* buff, size_t len) {
    untrusted_util::socket_send(socket, buff, len);
}

void ocall_socket_receive(int socket, void* buff, size_t len) {
    untrusted_util::socket_receive(socket, buff, len);
}

int ocall_open_uee_connection() {
    const char* host = UEE_HOSTNAME;
    const int port = UEE_PORT;

    return untrusted_util::socket_connect(host, port);
}

void ocall_uee_process(const int socket, void** out, size_t* out_len, const void* in, const size_t in_len) {
    untrusted_util::socket_send(socket, &in_len, sizeof(size_t));
    untrusted_util::socket_send(socket, in, in_len);

    untrusted_util::socket_receive(socket, out_len, sizeof(size_t));
    *out = malloc(*out_len);
    untrusted_util::socket_receive(socket, *out, *out_len);
}

void ocall_close_uee_connection(const int socket) {
    close(socket);
}

//DEBUG: for outside kmeans
float** all = NULL;
int count = 0;

int* ranges;

void ocall_set(size_t num_elems, float* buffer) {
    if(!all) {
        printf("all initialisation\n");
        all = new float*[5000000];
        ranges = new int[100];
    }

    for (unsigned i = 0; i < num_elems; ++i) {
        all[count] = (float*)malloc(64 * sizeof(float));
        memcpy(all[count], buffer + i * 64 * sizeof(float), 64 * sizeof(float));
        count++;
    }

    /*
    //printf("initialised\n");
    float* desc = (float*)malloc(num_elems * 64 * sizeof(float));
    memcpy(desc, buffer, num_elems * 64 * sizeof(float));
    all[count] = desc;
    ranges[count++] = count;
    printf("count %d\n", count);*/
}
int aaccess = 0;
float* ocall_get(const int pos) {
    if(aaccess++ % 10000000 == 0)
        printf("access %d\n", aaccess);

    return all[pos];
/*
    int sum = 0;
    int p = 0;
    while(pos < ranges[p]) {
        p++;
        sum += ranges[p];
    }
    int x = pos - sum;
    printf("count %d\n", count);
    return all[p] + 64 * sizeof(float) * x;*/
}
//DEBUG: for outside kmeans end

/***************************************************** ALLOCATORS *****************************************************/
void* ocall_untrusted_malloc(size_t length) {
    return malloc(length);
}

void ocall_untrusted_free(size_t pointer) {
    free((void*)pointer);
    //*pointer = NULL;
}
/*************************************************** END ALLOCATORS ***************************************************/

/****************************************************** GENERIC ******************************************************/
int ocall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    //extern_lib_ut::process_message(out, out_len, in, in_len); // must be implemented by extern lib
    //TODO uncomment and solve linking
    return 0;
}
/**************************************************** END GENERIC ****************************************************/
