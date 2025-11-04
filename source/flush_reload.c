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


int main(int argc, char *argv[]) {
    char leaked_secret[secret_length + 1];
    memset(leaked_secret, 0, (secret_length+1) * sizeof(char));

    // You need to properly set the following variables 
    int CACHE_THRESHOLD; // Should be easy once you do TASK 0
    int STRIDE;
    int ITERATIONS;
    int RELOADBUFFER_SIZE;
    unsigned char *reloadbuffer;

    /*
     * ================ TASK 1 ================
     *               FLUSH + RELOAD 
     * ========================================
     */

    printf("Secret: %s\n", leaked_secret);

    return 0;
}
