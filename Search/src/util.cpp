#include "util.h"

void ok_response(uint8_t** out, size_t* out_len) {
    *out_len = 1;
    unsigned char* ret = (unsigned char*)malloc(sizeof(unsigned char));
    ret[0] = 'x';
    *out = ret;
}
