//
//  Util.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 05/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "Utils.h"

//int denormalize(float val, int size) {
//    return round(val * size);
//}

long util_time_elapsed(struct timeval start, struct timeval end) {
  long secs_used,micros_used;

  secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  return micros_used;
}

void pee(const char *msg)
{
    perror(msg);
    exit(0);
}

void socketSend (int sockfd, unsigned char* buff, long size) {
    if (sendAll(sockfd, buff, size) < 0)
        pee("ERROR writing to socket");
}

int sendAll(int s, const void *buf, long len)
{
    long total = 0;       // how many bytes we've sent
    long bytesleft = len; // how many we have left to send
    long n = 0;

    while(total < len) {
        n = write(s, buf+total, bytesleft);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n==-1||total!=len ? -1 : 0; // return -1 on failure, 0 on success
}

void socketReceive(int sockfd, unsigned char* buff, long size) {
    if (receiveAll(sockfd, buff, size) < 0)
        pee("ERROR reading from socket");
}

int receiveAll (int socket, void *buff, long len) {
    int r = 0;
    while (r < len) {
        ssize_t n = read(socket,&((unsigned char *)buff)[r],len-r);
        if (n < 0) pee("ERROR reading from socket");
        r+=n;
    }
    return r;
}

void addToArr (void* val, int size, unsigned char* arr, int* pos) {
    memcpy(&arr[*pos], val, size);
    *pos += size;
}

void addIntToArr (int val, unsigned char* arr, int* pos) {
    addToArr (&val, sizeof(int), arr, pos);
}

void readFromArr (void* val, int size, unsigned char* arr, int* pos) {
    memcpy(val, &arr[*pos], size);
    *pos += size;
}

int readIntFromArr (unsigned char* arr, int* pos) {
    int x;
    readFromArr(&x, sizeof(int), arr, pos);
    return x;
}
