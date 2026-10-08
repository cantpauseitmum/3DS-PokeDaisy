#ifndef PD_GEN3_H
#define PD_GEN3_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_MOVES 4
#define MON_STRUCT_SIZE 100

typedef struct {
    uint16_t species;
    uint8_t level;
    uint16_t hp;
    uint16_t maxHp;
    uint16_t atk, def, speed, spatk, spdef;
    uint32_t status;
    uint16_t moves[NUM_MOVES];
    uint8_t pp[NUM_MOVES];
    uint32_t exp;
    
    // Advanced Stats
    uint8_t ivs[6]; // hp, atk, def, speed, spatk, spdef
    uint8_t evs[6];
    uint8_t nature;
    uint8_t abilityNum;
    uint8_t pokerus;
    uint8_t friendship;
    bool isEgg;
    
    uint32_t personality;
    uint32_t otId;
    
    bool isValid;
} Gen3PartyMon;

// Pass in the 100-byte raw array read from the emulator's memory
Gen3PartyMon PokeDaisy_DecodePartyMon(const uint8_t* raw);

#endif // PD_GEN3_H
