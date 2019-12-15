#ifndef _T_CRYPTO__H_
#define _T_CRYPTO__H_

#include <stdlib.h>
#include <stdint.h>
#include <sodium.h>

#define SHA256_OUTPUT_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

#define SODIUM_EXPBYTES (crypto_secretbox_NONCEBYTES+crypto_secretbox_MACBYTES)

// size of symmetric key
#define SYMM_KEY_SIZE (AES_KEY_SIZE)

// returns the encrypted size padded to the block size of the cipher
#define SYMM_ENC_SIZE(unenc_size) ((unenc_size) + (!((unenc_size) % AES_BLOCK_SIZE) ? 0 : (AES_BLOCK_SIZE - ((unenc_size) % AES_BLOCK_SIZE))))

namespace tcrypto {
    void random(void* out, size_t len);
    unsigned random_uint();
    unsigned random_uint_range(unsigned min, unsigned max);

    int sha256(unsigned char *out, const uint8_t* in, size_t len);
    int hmac_sha256(void* out, const void* in, const size_t in_len, const void* key, const size_t key_len);

    int encrypt(void* out, const uint8_t* in, const size_t in_len, const void* key, uint8_t* ctr);
    int decrypt(void* out, const uint8_t* in, const size_t in_len, const void* key, uint8_t* ctr);
}

#endif
