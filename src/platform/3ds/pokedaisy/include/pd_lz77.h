#ifndef PD_LZ77_H
#define PD_LZ77_H

#include <stdint.h>
#include <stddef.h>

// Decompress GBA LZ77 (type 0x10) data.
// Returns the decompressed size, or 0 on error.
// If dest is NULL, returns the required buffer size.
uint32_t PokeDaisy_LZ77Decompress(const uint8_t* src, size_t srcLen, uint8_t* dest);

#endif // PD_LZ77_H
