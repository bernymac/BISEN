#ifndef __IMGSRC_UTIL_H
#define __IMGSRC_UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include "outside_util.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

void ok_response(uint8_t** out, size_t* out_len);

#endif
