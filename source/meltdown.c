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

#include "asm.h"

#define SECRET_LENGTH 32

#define WOM_MAGIC_NUM 0x1337
#define WOM_GET_ADDRESS _IOR(WOM_MAGIC_NUM, 0, unsigned long)

void *wom_get_address(int fd) {
    void *addr = NULL;

    if (ioctl(fd, WOM_GET_ADDRESS, &addr) < 0)
        return NULL;

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

    /*
     * ================ TASK 2 ================
     *               MELTDOWN-TSX 
     * ========================================
     */

    printf("Secret: %s\n", leaked_secret);

    close(fd);

    return 0;

}
