#ifndef __IEEUTILS_H
#define __IEEUTILS_H

#include "vec_int.h"
//#include "ocall.h"

// IEE TOKEN DEFINITIONS
#define MAX_QUERY_TOKENS 25 // max tokens per query; vec_token should regrow anyway; iee-only restriction
//#define MAX_WORD_SIZE 128 // iee-only restriction; to simplify buffer handling

#define WORD_TOKEN 'w'
#define META_TOKEN 'z'

typedef struct iee_token {
    char type;
    unsigned counter;
    double idf;
    unsigned char* kW;
    vec_int docs;
    vec_int doc_frequencies;
} iee_token;
// END IEE TOKEN DEFINITIONS

void iee_addToArr(const void* val, int size, unsigned char* arr, int* pos);
void iee_readFromArr(const void* val, int size, const unsigned char* arr, int* pos);

void iee_addIntToArr(int val, unsigned char* arr, int* pos);
int iee_readIntFromArr(const unsigned char* arr, int* pos);
//void iee_add_ulonglong(unsigned long long val, unsigned char* arr, int* pos);
//unsigned long long iee_read_ulonglong(const unsigned char * arr, int* pos);

#endif /* __IEEUTILS_H */
