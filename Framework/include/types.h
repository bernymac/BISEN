#ifndef TYPES_H
#define TYPES_H

typedef long ssize_t;

typedef struct time_struct {
    long tv_sec;
    long tv_usec;
} untrusted_time;

#endif
