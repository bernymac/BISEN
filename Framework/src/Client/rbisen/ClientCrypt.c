//
//  IeeCrypt.c
//  BooleanSSE
//
//  Created by Guilherme Borges
//  Copyright © 2017 Guilherme Borges. All rights reserved.
//

#include "ClientCrypt.h"
#include <string.h>

void client_init_crypt() {
    // generate kF and Kenc
    client_kEnc = (unsigned char *)malloc(client_symBlocksize * sizeof(unsigned char));
    //client_c_random(client_kEnc, client_symBlocksize);
    memset(client_kEnc, 0x00, client_symBlocksize);

    client_kF = (unsigned char *)malloc(client_fBlocksize * sizeof(unsigned char));
    //client_c_random(client_kF, client_fBlocksize);
    memset(client_kF, 0x00, client_fBlocksize);
}

void client_destroy_crypt() {
    free(client_kEnc);
    free(client_kF);
}

unsigned char* client_get_kF() {
    return client_kF;
}

unsigned char* client_get_kEnc() {
    return client_kEnc;
}

void client_c_random(unsigned char *buf, unsigned long long len)
{
    randombytes_buf(buf, sizeof(unsigned char)*len);
}

unsigned int client_c_random_uint() {
    return randombytes_random();
}

unsigned int client_c_random_uint_range(int min, int max) {
    if(max < min)
        return max + (client_c_random_uint() % (int)(min - max));

    return min + (client_c_random_uint() % (int)(max - min));
}

// -1 failure, 0 ok
int client_c_encrypt(
  unsigned char *ciphertext, // message_len + C_EXPBYTES
  const unsigned char *message, // message_len
  unsigned long long message_len,
  const unsigned char *nonce, // C_NONCESIZE
  const unsigned char *key // C_KEYSIZE
)
{
  return crypto_secretbox_easy(ciphertext+crypto_secretbox_NONCEBYTES,
                               message, message_len,
                               nonce, key);
}

// -1 failure, 0 ok
int client_c_decrypt(
  unsigned char *decrypted, // ciphertext_len - C_EXPBYTES
  const unsigned char *ciphertext, // ciphertext_len
  unsigned long long ciphertext_len,
  const unsigned char *nonce, // C_NONCESIZE
  const unsigned char *key // C_KEYSIZE
)
{
  return crypto_secretbox_open_easy(decrypted, ciphertext+crypto_secretbox_NONCEBYTES,
                                    ciphertext_len-crypto_secretbox_NONCEBYTES,
                                    nonce, key);
}

// -1 failure, 0 ok
int client_c_hmac(
  unsigned char *out, // H_BYTES
  const unsigned char *in, // inlen
  unsigned long long inlen,
  const unsigned char *k // H_KEYSIZE
)
{
  return crypto_auth_hmacsha256(out, in, inlen, k);
}

// -1 failure, 0 ok
int client_c_hmac_verify(const unsigned char *h, const unsigned char *in, unsigned long long inlen, const unsigned char *k)
{
  return crypto_auth_hmacsha256_verify(h, in, inlen, k);
}
