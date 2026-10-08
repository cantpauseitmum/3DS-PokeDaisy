#include "include/pd_lz77.h"

uint32_t PokeDaisy_LZ77Decompress(const uint8_t* src, size_t srcLen, uint8_t* dest) {
    if (srcLen < 4) return 0;
    
    if (src[0] != 0x10) return 0; // Not GBA LZ77
    
    uint32_t decompressedSize = src[1] | (src[2] << 8) | (src[3] << 16);
    if (decompressedSize == 0) return 0;
    if (dest == NULL) return decompressedSize;
    
    uint32_t srcPos = 4;
    uint32_t destPos = 0;
    
    while (srcPos < srcLen && destPos < decompressedSize) {
        uint8_t flags = src[srcPos++];
        
        for (int i = 0; i < 8; i++) {
            if (flags & 0x80) { // Compressed block
                if (srcPos + 1 >= srcLen) break;
                uint8_t block1 = src[srcPos++];
                uint8_t block2 = src[srcPos++];
                
                uint32_t length = (block1 >> 4) + 3;
                uint32_t offset = (((block1 & 0x0F) << 8) | block2) + 1;
                
                uint32_t copySrc = destPos - offset;
                for (uint32_t j = 0; j < length && destPos < decompressedSize; j++) {
                    if (copySrc >= 0 && copySrc < destPos) {
                        dest[destPos++] = dest[copySrc++];
                    } else {
                        dest[destPos++] = 0; // Error case, just pad
                    }
                }
            } else { // Uncompressed byte
                if (srcPos >= srcLen) break;
                dest[destPos++] = src[srcPos++];
            }
            flags <<= 1;
            if (destPos >= decompressedSize) break;
        }
    }
    
    return destPos;
}
