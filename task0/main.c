#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <x86intrin.h>
#include "asm.h"

#define CACHELINE 64
#define NUM_SAMPLES 10000
#define ROUNDS 5

char* buffer;

size_t cache_hit(void) {
    size_t hit_time = 0;
    size_t min_time = SIZE_MAX;
    for (size_t i = 0; i < ROUNDS; i++) {
        *(volatile char*)buffer;
        mfence();
        cpuid();
        uint64_t t1 = rdtscp();
        *(volatile char*)buffer;
        uint64_t t2 = rdtscp();
        cpuid();
        if (t2 - t1 < min_time) {
            min_time = t2 - t1;
        }
    }
}

size_t cache_miss(void) {
    size_t miss_time = 0;
    size_t min_time = SIZE_MAX;
    for (size_t i = 0; i < ROUNDS; i++) {
        clflush(buffer);
        mfence();
        cpuid();
        uint64_t t1 = rdtscp();
        *(volatile char*)buffer;
        uint64_t t2 = rdtscp();
        cpuid();
        if (t2 - t1 < min_time) {
            min_time = t2 - t1;
        }
    }
}

int main(int argc, char** argv) {
    // allocate a buffer of size cache line aligned to cache line
    buffer = (char*) aligned_alloc(CACHELINE, CACHELINE);
    assert(buffer != NULL);
    // cahce hit
    for (size_t i = 0; i < NUM_SAMPLES; i++) {
        size_t hit_time = cache_hit();
        if (hit_time < 400)
            printf("hit,%zu\n", hit_time);
    }

    // cache miss
    for (size_t i = 0; i < NUM_SAMPLES; i++) {
        size_t miss_time = cache_miss();
        if (miss_time < 400)
            printf("miss,%zu\n", miss_time);
    }
    return 0;
}