//
//  Utils.h
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 04/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//
#ifndef __BooleanSSE__Utils__
#define __BooleanSSE__Utils__

#include <pthread.h>
#include <dirent.h>
#include <stdint.h>
#include <math.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "zlib.h"
#include "Definitions.h"

#define  LOGI(...)  fprintf(stdout,__VA_ARGS__)

long util_time_elapsed (struct timeval start, struct timeval end);

void pee(const char *msg);

int sendAll(int s, const void *buf, long len);
int receiveAll (int s, void *buff, long len);

//int connectAndSend (unsigned char* buff, long size);

void socketSend (int sockfd, unsigned char* buff, long size);
void socketReceive(int sockfd, unsigned char* buff, long size);

//void socketReceiveAck(int sockfd);

void addToArr (void* val, int size, unsigned char* arr, int* pos);
void readFromArr (void* val, int size, unsigned char* arr, int* pos);

void addIntToArr (int val, unsigned char* arr, int* pos);
int readIntFromArr (unsigned char* arr, int* pos);

#endif /* defined(__BooleanSSE__ClientUtils__) */
