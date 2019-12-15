#include "untrusted_crypto.h"

#include <stdlib.h>
#include <stdio.h> // for debug purposes
#include <stdint.h>
#include <string.h>

#include <openssl/aes.h>
#include <openssl/rand.h>

void utcrypto::encrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr) {
    /*AES_KEY aes_key;
    if(AES_set_encrypt_key(key, 128, &aes_key)) {
        printf("AES_set_encrypt_key error.\n");
        exit(-1);
    }

    unsigned num = 0;
    unsigned char ecount[AES_BLOCK_SIZE];
    memset(ecount, 0x00, AES_BLOCK_SIZE);

    AES_ctr128_encrypt(in, out, in_len, &aes_key, ctr, ecount, &num);*/
}

void utcrypto::decrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr) {
    /*AES_KEY aes_key;
    if (AES_set_encrypt_key(key, 128, &aes_key)){
        printf("AES_set_encrypt_key error.\n");
        exit(-1);
    }

    unsigned num = 0;
    unsigned char ecount[AES_BLOCK_SIZE];
    memset(ecount, 0x00, AES_BLOCK_SIZE);

    AES_ctr128_encrypt(in, out, in_len, &aes_key, ctr, ecount, &num);*/
}

int utcrypto::sodium_encrypt(
        unsigned char *ciphertext, // message_len + C_EXPBYTES
        const unsigned char *message, // message_len
        unsigned long long message_len,
        const unsigned char *nonce, // C_NONCESIZE
        const unsigned char *key // C_KEYSIZE
)
{
    return crypto_secretbox_easy(ciphertext+crypto_secretbox_NONCEBYTES, message, message_len, nonce, key);
}

// -1 failure, 0 ok
int utcrypto::sodium_decrypt(
        unsigned char *decrypted, // ciphertext_len - C_EXPBYTES
        const unsigned char *ciphertext, // ciphertext_len
        unsigned long long ciphertext_len,
        const unsigned char *nonce, // C_NONCESIZE
        const unsigned char *key // C_KEYSIZE
)
{
    return crypto_secretbox_open_easy(decrypted, ciphertext+crypto_secretbox_NONCEBYTES, ciphertext_len-crypto_secretbox_NONCEBYTES, nonce, key);
}
