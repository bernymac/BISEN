#include "vec_int.h"

void vi_init(vec_int* v, int max_size) {
    v->counter = 0;
    v->max_size = max_size;

    // allocation
    v->array = (int*) malloc(sizeof(int) * v->max_size);
    //for(int i = 0; i < v->max_size; i++)
    //    v->array[i] = 0;
}

void vi_destroy(vec_int* v) {
    free(v->array);
}

void vi_push_back(vec_int* v, int e) {
    if(v->counter < v->max_size)
        v->array[v->counter++] = e;
}

unsigned vi_size(vec_int* v) {
    return v->counter;
}

int vi_index_of(vec_int v, int e) {
    for(unsigned i = 0; i < v.counter; i++) {
        if(v.array[i] == e)
            return i;
    }

    return -1;
}

vec_int vi_vec_union(vec_int a, vec_int b, unsigned char* count, unsigned ndocs) {
    memset(count, 0, sizeof(unsigned char) * ndocs);

    unsigned ops = a.counter;

    // add all elements from a
    for(unsigned i = 0; i < a.counter; i++) {
        count[a.array[i]] = 1;
    }

    //int nops = 0;
    // add all elements from b, if they're not in the union yet
    for(unsigned i = 0; i < b.counter; i++) {
        count[b.array[i]]++;
        if(count[b.array[i]] == 1)
            ops++;
    }

    vec_int v;
    vi_init(&v, ops);

    for(unsigned i = 0; i < ndocs; i++) {
        if(count[i] > 0) {
            vi_push_back(&v, i);
        }
    }

    return v;
}

vec_int vi_vec_intersection(vec_int a, vec_int b, unsigned char* count, unsigned ndocs) {
    memset(count, 0, sizeof(unsigned char) * ndocs);
    unsigned int ops = 0;

    // add all elements from a
    for(unsigned i = 0; i < a.counter; i++) {
        count[a.array[i]] = 1;
    }

    // add all elements from b
    for(unsigned i = 0; i < b.counter; i++) {
        count[b.array[i]]++;
        if(count[b.array[i]] == 2)
            ops++;
    }

    vec_int v;
    vi_init(&v, ops);

    for(unsigned i = 0; i < ndocs; i++) {
        if(count[i] == 2) {
            vi_push_back(&v, i);
        }
    }

    return v;
}

int max(int a, int b)
{
    return a < b? b : a;
}
