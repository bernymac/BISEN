#ifndef RBISENSSE_IEE_H
#define RBISENSSE_IEE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h" // mpc data types

#define OP_SETUP 0x69
#define OP_ADD 0x61
#define OP_SRC 0x73

#define RES_OK 0x90

void f(unsigned char** out, size* out_len, const unsigned long long pid, uint8_t* in, const size in_len);

#endif
