#ifndef __UNTRUSTED_UTIL_H
#define __UNTRUSTED_UTIL_H

#include <stdlib.h>
#include <stdint.h>

typedef void* secure_connection;

namespace untrusted_util {
    struct timeval curr_time();
    double time_elapsed_ms(struct timeval start, struct timeval end);
    void debug_printbuf(uint8_t* buf, size_t len);

    int init_server(const int server_port);

    int socket_connect(const char* server_name, const int server_port);
    void socket_send(int socket, const void* buff, size_t len);
    void socket_receive(int socket, void* buff, size_t len);

    void init_secure_connection(secure_connection** conn, const char* server_name, const int server_port);
    void socket_secure_send(secure_connection* conn, const void* buff, size_t len);
    void socket_secure_receive(secure_connection* conn, void* buff, size_t len);
    void close_secure_connection(secure_connection* conn);
}

#endif
