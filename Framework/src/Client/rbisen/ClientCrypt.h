//
//  IeeCrypt.h
//  BooleanSSE
//
//  Created by Guilherme Borges.
//  Copyright © 2017 Guilherme Borges. All rights reserved.
//

#ifndef ClientCrypt_h
#define ClientCrypt_h

#include <sodium.h>

#define C_KEYSIZE   crypto_secretbox_KEYBYTES
#define C_NONCESIZE crypto_secretbox_NONCEBYTES
#define C_EXPBYTES (crypto_secretbox_NONCEBYTES+crypto_secretbox_MACBYTES)

#define H_KEYSIZE crypto_auth_hmacsha256_KEYBYTES
#define H_BYTES crypto_auth_hmacsha256_BYTES

#ifdef _cplusplus
extern "C" {
#error
#endif

const size_t client_symBlocksize = crypto_secretbox_KEYBYTES; /* changed to fix valgrind warning : needs 32 bytes key CHECKME */
const size_t client_fBlocksize = crypto_auth_hmacsha256_KEYBYTES;

// keys sent by the client
static unsigned char* client_kEnc;
static unsigned char* client_kF;

void client_init_crypt();
void client_destroy_crypt();

unsigned char* client_get_kF();
unsigned char* client_get_kEnc();

// RANDOM FUNCTIONS
void client_c_random(unsigned char *buf, unsigned long long len);
unsigned int client_c_random_uint();
unsigned int client_c_random_uint_range(int min, int max);

// CRYPTO FUNCTIONS
// -1 failure, 0 ok

int client_c_encrypt(
  unsigned char *ciphertext, // message_len + C_EXPBYTES
  const unsigned char *message, // message_len
  unsigned long long message_len,
  const unsigned char *nonce, // C_NONCESIZE
  const unsigned char *key // C_KEYSIZE
);

int client_c_decrypt(
  unsigned char *decrypted, // ciphertext_len - C_EXPBYTES
  const unsigned char *ciphertext, // ciphertext_len
  unsigned long long ciphertext_len,
  const unsigned char *nonce, // C_NONCESIZE
  const unsigned char *key // C_KEYSIZE
);

int client_c_hmac(
  unsigned char *out, // H_BYTES
  const unsigned char *in, // inlen
  unsigned long long inlen,
  const unsigned char *k // H_KEYSIZE
);

int client_c_hmac_verify(
  const unsigned char *h, // H_BYTES
  const unsigned char *in, // inlen
  unsigned long long inlen,
  const unsigned char *k // H_KEYSIZE
);

#ifdef _cplusplus
}
# endif

#endif /* ClientCrypt_h */
