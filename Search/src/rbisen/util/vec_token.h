#ifndef vec_token_H
#define vec_token_H

#include <stdlib.h> // malloc / free
#include "IeeUtils.h"

typedef struct vec_token {
    iee_token* array;
    unsigned int max_size;
    unsigned int counter;
} vec_token;

// initialisers
void vt_init(vec_token* v, int max_size);
void vt_grow(vec_token* v);
void vt_destroy(vec_token* v);

// modifiers
void vt_push_back(vec_token* v, iee_token* e);
void vt_pop_back(vec_token* v);

// elements access
iee_token vt_peek_back(vec_token v);
unsigned vt_size(vec_token* v);

#endif
