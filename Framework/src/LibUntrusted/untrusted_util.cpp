#include "untrusted_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>

static int send_all(int socket, const void* buf, size_t len) {
    size_t total = 0;        // how many bytes we've sent
    size_t bytesleft = len; // how many we have left to send
    long n = 0;

    while(total < len) {
        n = write(socket, (unsigned char*)buf + total, bytesleft);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n == -1 || total != len ? -1 : 0; // return -1 on failure, 0 on success
}

static ssize_t receive_all(int socket, void* buff, size_t len) {
    ssize_t r = 0;
    while ((unsigned long)r < len) {
        //printf("calling len %lu; r %ld\n", len, r);
        ssize_t n = read(socket, (unsigned char*)buff + r, len-r);
        //printf("got %lu\n-------------------------\n", n);
        if (n < 0) {
            printf("ERROR reading from socket\n");
            exit(1);
        }
        r+=n;
    }

    return r;
}

void untrusted_util::socket_send(int socket, const void* buff, size_t len) {
    if (send_all(socket, buff, len) < 0) {
        printf("ERROR writing to socket\n");
        exit(1);
    }
}

void untrusted_util::socket_receive(int socket, void* buff, size_t len) {
    if (receive_all(socket, buff, len) < 0) {
        printf("EERROR reading from socket\n");
        exit(1);
    }
}

int untrusted_util::init_server(const int server_port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listen_socket;
    if ((listen_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Could not create socket!\n");
        exit(1);
    }

    int res = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &res, sizeof(res)) == -1) {
        printf("Could not set socket options!\n");
        exit(1);
    }

    if ((bind(listen_socket, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
        printf("Could not bind socket!\n");
        exit(1);
    }

    if (listen(listen_socket, 16) < 0) {
        printf("Could not open socket for listening!\n");
        exit(1);
    }

    return listen_socket;
}

int untrusted_util::socket_connect(const char* server_name, const int server_port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_name, &server_addr.sin_addr);
    server_addr.sin_port = htons(server_port);

    // open a stream socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Could not create client socket!\n");
        exit(1);
    }

    //int flag = 1;
    //setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

    //int iMode = 0;
    //ioctl(sock, FIONBIO, &iMode);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Could not connect to server!\n");
        exit(1);
    }

    return sock;
}

void untrusted_util::debug_printbuf(uint8_t* buf, size_t len) {
    for (size_t l = 0; l < len; ++l)
        printf("%02x ", buf[l]);
    printf("\n");
}

struct timeval untrusted_util::curr_time() {
    struct timeval curr;
    gettimeofday(&curr, NULL);
    return curr;
}

double untrusted_util::time_elapsed_ms(struct timeval start, struct timeval end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return (double)micros_used / 1000.0;
}
