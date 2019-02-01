#ifndef _BDAP_ENCRYPTION_OS_RAND_H
#define _BDAP_ENCRYPTION_OS_RAND_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialises random number generator.
 * 
 * @note This is a no-op method, only a place-holder.
 * 
 * @param seed the pointer to the seed
 * @param seed_size the size of the seed in bytes
 */
void os_randominit(const uint8_t* seed, size_t seed_size);

/**
 * @brief Randomly generates a block of {@code buf_size} bytes
 * using native OS random-number generator.
 * 
 * @param buf The pointer to the generated random block
 * @param buf_size The length of the random block in bytes
 */
void os_randombytes(uint8_t *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif
