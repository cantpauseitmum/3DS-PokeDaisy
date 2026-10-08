#include "include/pd_gen3.h"
#include <string.h>

static const int gen3SubstructOrder[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};

Gen3PartyMon PokeDaisy_DecodePartyMon(const uint8_t* raw) {
    Gen3PartyMon mon = { .isValid = false };
    
    uint32_t personality;
    memcpy(&personality, raw + 0, 4);
    
    uint32_t otId;
    memcpy(&otId, raw + 4, 4);
    
    uint16_t storedChecksum;
    memcpy(&storedChecksum, raw + 28, 2);
    
    uint32_t key = personality ^ otId;

    uint32_t block[12];
    uint32_t sum = 0;
    
    for (int i = 0; i < 12; i++) {
        uint32_t encWord;
        memcpy(&encWord, raw + 32 + (i * 4), 4);
        
        block[i] = encWord ^ key;
        sum = (sum + (block[i] & 0xFFFF) + ((block[i] >> 16) & 0xFFFF)) & 0xFFFF;
    }
    
    if (sum != storedChecksum || storedChecksum == 0) {
        return mon; // invalid checksum or empty slot
    }
    
    const int* order = gen3SubstructOrder[personality % 24];
    int growthOff = 0, attacksOff = 0, evsOff = 0, miscOff = 0;
    
    for (int slot = 0; slot < 4; slot++) {
        switch (order[slot]) {
            case 0: growthOff = slot * 12; break; // byte offset
            case 1: attacksOff = slot * 12; break;
            case 2: evsOff = slot * 12; break;
            case 3: miscOff = slot * 12; break;
        }
    }
    
    uint8_t* byteBlock = (uint8_t*)block;
    
    memcpy(&mon.species, byteBlock + growthOff, 2);
    memcpy(&mon.exp, byteBlock + growthOff + 4, 4);
    
    for (int i = 0; i < NUM_MOVES; i++) {
        memcpy(&mon.moves[i], byteBlock + attacksOff + (i * 2), 2);
        mon.pp[i] = byteBlock[attacksOff + 8 + i];
    }
    
    for (int i = 0; i < 6; i++) {
        mon.evs[i] = byteBlock[evsOff + i];
    }
    
    mon.friendship = byteBlock[growthOff + 9];
    mon.pokerus = byteBlock[miscOff + 0];
    
    uint32_t ivData;
    memcpy(&ivData, byteBlock + miscOff + 4, 4);
    mon.ivs[0] = (ivData >> 0) & 0x1F;
    mon.ivs[1] = (ivData >> 5) & 0x1F;
    mon.ivs[2] = (ivData >> 10) & 0x1F;
    mon.ivs[3] = (ivData >> 15) & 0x1F;
    mon.ivs[4] = (ivData >> 20) & 0x1F;
    mon.ivs[5] = (ivData >> 25) & 0x1F;
    mon.isEgg = (ivData >> 30) & 1;
    mon.abilityNum = (ivData >> 31) & 1;
    mon.nature = personality % 25;
    
    mon.level = raw[0x54];
    memcpy(&mon.hp, raw + 0x56, 2);
    memcpy(&mon.maxHp, raw + 0x58, 2);
    memcpy(&mon.atk, raw + 0x5A, 2);
    memcpy(&mon.def, raw + 0x5C, 2);
    memcpy(&mon.speed, raw + 0x5E, 2);
    memcpy(&mon.spatk, raw + 0x60, 2);
    memcpy(&mon.spdef, raw + 0x62, 2);
    memcpy(&mon.status, raw + 0x50, 4);
    
    mon.personality = personality;
    mon.otId = otId;
    mon.isValid = true;
    
    return mon;
}
