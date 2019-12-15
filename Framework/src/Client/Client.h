#ifndef _CLIENT_H
#define _CLIENT_H

#include "untrusted_crypto.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct img_descriptor {
    unsigned count;
    float* descriptors;
} img_descriptor;

#endif
