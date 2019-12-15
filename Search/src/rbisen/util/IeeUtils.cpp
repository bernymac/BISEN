#include "IeeUtils.h"

void iee_addToArr(const void* val, int size, unsigned char* arr, int* pos) {
    memcpy(&arr[*pos], val, size);
    *pos += size;
}

void iee_readFromArr(const void* val, int size, const unsigned char * arr, int* pos) {
    memcpy((void*)val, &arr[*pos], size);
    *pos += size;
}

void iee_addIntToArr(int val, unsigned char* arr, int* pos) {
    iee_addToArr (&val, sizeof(int), arr, pos);
}

int iee_readIntFromArr(const unsigned char * arr, int* pos) {
    int x;
    iee_readFromArr(&x, sizeof(int), arr, pos);
    return x;
}
