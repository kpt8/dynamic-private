#ifndef _BDAP_ENCRYPTION_AES256_H
#define _BDAP_ENCRYPTION_AES256_H

#include <stdint.h>

#define AES256_KEY_SIZE     32

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bit-sliced implementation of AES256 encryption engine.
 * 
 * @param out The output ciphertext block, 16 bytes
 * @param in The input plaintext block, 16 bytes
 * @param key The encryption key, 32 bytes
 */
void aes256_bitslice_encrypt(uint8_t *out, 
                             const uint8_t *in,
                             const uint8_t *key);

/**
 * @brief Bit-sliced implementation of AES256 decryption engine.
 * 
 * @param out The output plaintext block, 16 bytes
 * @param in The input ciphertext block, 16 bytes
 * @param key The decryption key, 32 bytes
 */
void aes256_bitslice_decrypt(uint8_t *out,
                             const uint8_t *in,
                             const uint8_t *key);

#ifdef __cplusplus
}
#endif

#endif //_BDAP_ENCRYPTION_AES256_H
