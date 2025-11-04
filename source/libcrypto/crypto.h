#ifndef __CRYPTO__
#define __CRYPTO__
 
#ifndef SECRET
uint64_t secret = 0; // the real secret value is set in the library
#else
uint64_t secret = SECRET; // the user provided SECRET set in CFLAGS
#endif

int secret_length = sizeof(secret);

/*
 * This function accesses (i.e. brings to the cache) the entry of buff 
 * at offset secret[index] with the provided stride
 */
extern void encrypt_secret_byte(unsigned char* buff, size_t stride, size_t index);
 
#endif
