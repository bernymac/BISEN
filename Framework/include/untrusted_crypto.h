#ifndef _UT_CRYPTO__H_
#define _UT_CRYPTO__H_

#include <stdlib.h>
#include <stdint.h>
#include <sodium.h>

#define SHA256_OUTPUT_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

#define SODIUM_EXPBYTES (crypto_secretbox_NONCEBYTES+crypto_secretbox_MACBYTES)

namespace utcrypto {
    void encrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr);
    void decrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr);

    int sodium_encrypt(unsigned char *ciphertext,const unsigned char *message, unsigned long long message_len, const unsigned char *nonce, const unsigned char *key);
    int sodium_decrypt(unsigned char *ciphertext,const unsigned char *message, unsigned long long message_len, const unsigned char *nonce, const unsigned char *key);
}

#endif
