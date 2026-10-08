#ifndef PD_CONFIG_H
#define PD_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * POCKET_ORDER definitions
 */
typedef enum {
    POCKET_ITEMS = 0,
    POCKET_POKE_BALLS = 1,
    POCKET_TM_HM = 2,
    POCKET_BERRIES = 3,
    POCKET_KEY_ITEMS = 4,
} BagPocketType;

/**
 * NativeConfig defines all the specific RAM offsets and pointers 
 * for a specific Gen 3 ROM or ROM hack.
 * Translated from the Android Kotlin codebase (NativeReader.kt).
 */
typedef struct {
    // Core Game Data
    uint32_t playerParty;
    uint32_t playerPartyCount;
    uint32_t battleMons;
    uint32_t battlerPositions;
    uint32_t battlersCount;
    uint32_t battleTypeFlags;
    
    // Core Pointers
    uint32_t gMain;
    uint32_t saveBlock1Ptr;
    uint32_t saveBlock2Ptr;
    
    // Map & Objects
    uint32_t objectEvents;
    uint32_t mapHeader;
    
    // Bag & Items
    uint32_t bagPockets;
    int bagPocketCount;
    int32_t encryptionKeyOff;  // Offset in SaveBlock2. -1 if none.
    BagPocketType bagPocketOrder[6];
    
    // Icon Tables (ROM addresses)
    uint32_t monIconTable;
    uint32_t monIconPaletteTable;
    uint32_t monIconPaletteIndices;
    uint32_t itemIconTable;
    
    // State Offsets
    bool staticSaveBlocks;     // Ruby/Sapphire don't use pointers for saveblocks
    bool battleStructStatic;
    int32_t moneyOff;          // Offset in SaveBlock1. -1 if none.
    uint32_t inBattleOff;      // Offset from gMain
    
    // Battle specific
    uint32_t enemyParty;
    uint32_t battlerPartyIndexes;
    uint32_t battleStructPtr;
    uint32_t monToSwitchIntoOff;
    
    // UI Touch Hooks (ROM addresses)
    uint32_t battlerControllerFuncs;
    uint32_t handleInputChooseAction;
    uint32_t handleInputChooseMove;
    uint32_t completeWhenChoseItem;
    uint32_t waitForMonSelection;
    uint32_t handleInputChooseTarget;
    
    // Region Map (ROM addresses)
    uint32_t regionMapGfx;
    uint32_t regionMapPal;
    uint32_t regionMapTilemap;

    // Metadata
    char language;
    const char* gameCode;
} NativeConfig;


// ------------------------------------------------------------------
// CONFIGURATIONS
// ------------------------------------------------------------------

static const BagPocketType FIRERED_BAG_POCKET_ORDER[] = {
    POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES
};

static const BagPocketType EMERALD_BAG_POCKET_ORDER[] = {
    POCKET_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES, POCKET_KEY_ITEMS
};

/**
 * Pokémon FireRed (BPRE) Rev 0
 */
static const NativeConfig NATIVE_FIRERED_REV0 = {
    .playerParty = 0x02024284,
    .playerPartyCount = 0x02024029,
    .battleMons = 0x02023BE4,
    .battlerPositions = 0x02023BD6,
    .battlersCount = 0x02023BCC,
    .battleTypeFlags = 0x02022B4C,
    .gMain = 0x030030F0,
    .saveBlock1Ptr = 0x03005008,
    .saveBlock2Ptr = 0x0300500C,
    .objectEvents = 0x02036E38,
    .mapHeader = 0x02036DFC,
    .bagPockets = 0x0203988C,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xF20,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES },
    .moneyOff = 0x290,
    
    .battlerControllerFuncs = 0x03004fe0,
    .handleInputChooseAction = 0x0802e438,
    .handleInputChooseMove = 0x0802ea10,
    .completeWhenChoseItem = 0x0803073c,
    .waitForMonSelection = 0x08030684,
    .handleInputChooseTarget = 0x0802e674,
    
    .regionMapGfx = 0x080C0330,
    .regionMapPal = 0x080C02EC,
    .regionMapTilemap = 0x080C035C,
    
    .monIconTable = 0x083D37A0,
    .monIconPaletteIndices = 0x083D3E80,
    .monIconPaletteTable = 0x083D4038,
    .itemIconTable = 0x083D4294,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    .enemyParty = 0,
    .language = 'E',
    .gameCode = "BPRE"
};

/**
 * Pokémon LeafGreen (BPGE) Rev 0
 */
static const NativeConfig NATIVE_LEAFGREEN_REV0 = {
    .playerParty = 0x02024284,
    .playerPartyCount = 0x02024029,
    .battleMons = 0x02023BE4,
    .battlerPositions = 0x02023BD6,
    .battlersCount = 0x02023BCC,
    .battleTypeFlags = 0x02022B4C,
    .gMain = 0x030030F0,
    .saveBlock1Ptr = 0x03005008,
    .saveBlock2Ptr = 0x0300500C,
    .objectEvents = 0x02036E38,
    .mapHeader = 0x02036DFC,
    .bagPockets = 0x0203988C,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xF20,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES },
    .moneyOff = 0x290,
    
    .monIconTable = 0x083D35DC,
    .monIconPaletteIndices = 0x083D3CBC,
    .monIconPaletteTable = 0x083D3E74,
    .itemIconTable = 0x083D40D0,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    .enemyParty = 0,
    .language = 'E',
    .gameCode = "BPGE"
};

/**
 * Pokémon Radical Red v4.1 (BPRE 32MB)
 */
static const NativeConfig NATIVE_RADICAL_RED = {
    .playerParty = 0x02024284,
    .playerPartyCount = 0x02024029,
    .battleMons = 0x02023BE4,
    .battlerPositions = 0x02023BD6,
    .battlersCount = 0x02023BCC,
    .battleTypeFlags = 0x02022B4C,
    .gMain = 0x030030F0,
    .saveBlock1Ptr = 0x03005008,
    .saveBlock2Ptr = 0x0300500C,
    .objectEvents = 0x02036E38,
    .mapHeader = 0x02036DFC,
    .bagPockets = 0x0203988C,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xF20,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES },
    .moneyOff = 0x290,
    
    .monIconTable = 0x097FE6CC,
    .monIconPaletteIndices = 0x097FE164,
    .monIconPaletteTable = 0x083D4038,
    .itemIconTable = 0x093C8100,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    .enemyParty = 0,
    .language = 'E',
    .gameCode = "BPRE"
};

/**
 * Pokémon FireRed (BPRE) Rev 1
 */
static const NativeConfig NATIVE_FIRERED_REV1 = {
    .playerParty = 0x02024284,
    .playerPartyCount = 0x02024029,
    .battleMons = 0x02023BE4,
    .battlerPositions = 0x02023BD6,
    .battlersCount = 0x02023BCC,
    .battleTypeFlags = 0x02022B4C,
    .gMain = 0x030030F0,
    .saveBlock1Ptr = 0x03005008,
    .saveBlock2Ptr = 0x0300500C,
    .objectEvents = 0x02036E38,
    .mapHeader = 0x02036DFC,
    .bagPockets = 0x0203988C,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xF20,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES },
    .moneyOff = 0x290,
    
    .battlerControllerFuncs = 0x03004fe0,
    .handleInputChooseAction = 0x0802e44c,
    .handleInputChooseMove = 0x0802ea24,
    .completeWhenChoseItem = 0x08030750,
    .waitForMonSelection = 0x08030698,
    .handleInputChooseTarget = 0x0802e688,
    
    .regionMapGfx = 0x080C0330, // Assuming Rev1 same
    .regionMapPal = 0x080C02EC,
    .regionMapTilemap = 0x080C035C,
    
    .monIconTable = 0x083D3810,
    .monIconPaletteIndices = 0x083D3EF0,
    .monIconPaletteTable = 0x083D40A8,
    .itemIconTable = 0x083D4304,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    
    .enemyParty = 0x0202402C,
    .battlerPartyIndexes = 0x02023BCE,
    .battleStructPtr = 0x02023FE8,
    .monToSwitchIntoOff = 0x5C,
    .language = 'E',
    .gameCode = "BPRE"
};

/**
 * Pokémon LeafGreen (BPGE) Rev 1
 */
static const NativeConfig NATIVE_LEAFGREEN_REV1 = {
    .playerParty = 0x02024284,
    .playerPartyCount = 0x02024029,
    .battleMons = 0x02023BE4,
    .battlerPositions = 0x02023BD6,
    .battlersCount = 0x02023BCC,
    .battleTypeFlags = 0x02022B4C,
    .gMain = 0x030030F0,
    .saveBlock1Ptr = 0x03005008,
    .saveBlock2Ptr = 0x0300500C,
    .objectEvents = 0x02036E38,
    .mapHeader = 0x02036DFC,
    .bagPockets = 0x0203988C,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xF20,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_KEY_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES },
    .moneyOff = 0x290,
    
    .monIconTable = 0x083D364C,
    .monIconPaletteIndices = 0x083D3D2C,
    .monIconPaletteTable = 0x083D3EE4,
    .itemIconTable = 0x083D4140,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    .enemyParty = 0,
    .language = 'E',
    .gameCode = "BPGE"
};

/**
 * Pokémon Emerald (BPEE)
 */
static const NativeConfig NATIVE_EMERALD = {
    .playerParty = 0x020244EC,
    .playerPartyCount = 0x020244E9,
    .battleMons = 0x02024084,
    .battlerPositions = 0x02024076,
    .battlersCount = 0x0202406C,
    .battleTypeFlags = 0x02022FEC,
    .gMain = 0x030022C0,
    .saveBlock1Ptr = 0x03005D8C,
    .saveBlock2Ptr = 0x03005D90,
    .objectEvents = 0x02037350,
    .mapHeader = 0x02037318,
    .bagPockets = 0x02039DD8,
    .bagPocketCount = 5,
    .encryptionKeyOff = 0xAC,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES, POCKET_KEY_ITEMS },
    .moneyOff = 0x490,
    
    .battlerControllerFuncs = 0x03005d60,
    .handleInputChooseAction = 0x08057588,
    .handleInputChooseMove = 0x08057bfc,
    .completeWhenChoseItem = 0x080598e0,
    .waitForMonSelection = 0x08059828,
    .handleInputChooseTarget = 0x08057824,
    
    .monIconTable = 0x0857BCA8,
    .monIconPaletteIndices = 0x0857C388,
    .monIconPaletteTable = 0x0857C540,
    .itemIconTable = 0x08614410,
    
    .staticSaveBlocks = false,
    .battleStructStatic = false,
    .inBattleOff = 0x439,
    
    .enemyParty = 0x02024744,
    .battlerPartyIndexes = 0x0202406E,
    .battleStructPtr = 0x0202449C,
    .monToSwitchIntoOff = 0x5C,
    .language = 'E',
    .gameCode = "BPEE"
};

/**
 * Pokémon Ruby (AXVE)
 */
static const NativeConfig NATIVE_RUBY = {
    .playerParty = 0x03004360,
    .playerPartyCount = 0x03004350,
    .battleMons = 0x02024A80,
    .battlerPositions = 0x02024A72,
    .battlersCount = 0x02024A68,
    .battleTypeFlags = 0x020239F8,
    .gMain = 0x03001770,
    .saveBlock1Ptr = 0x02025734,
    .saveBlock2Ptr = 0x02024EA4,
    .objectEvents = 0x030048A0,
    .mapHeader = 0x0202E828,
    .bagPockets = 0x083C1634,
    .bagPocketCount = 5,
    .encryptionKeyOff = -1,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES, POCKET_KEY_ITEMS },
    .moneyOff = 0x490,
    
    .monIconTable = 0x083BBD3C,
    .monIconPaletteIndices = 0x083BC41C,
    .monIconPaletteTable = 0x083BC5D4,
    .itemIconTable = 0,
    
    .staticSaveBlocks = true,
    .battleStructStatic = true,
    .inBattleOff = 0x43D,
    
    .enemyParty = 0x030045C0,
    .battlerPartyIndexes = 0x02024A6A,
    .battleStructPtr = 0x02000000,
    .monToSwitchIntoOff = 0x16068,
    .language = 'E',
    .gameCode = "AXVE"
};

/**
 * Pokémon Sapphire (AXPE)
 */
static const NativeConfig NATIVE_SAPPHIRE = {
    .playerParty = 0x03004360,
    .playerPartyCount = 0x03004350,
    .battleMons = 0x02024A80,
    .battlerPositions = 0x02024A72,
    .battlersCount = 0x02024A68,
    .battleTypeFlags = 0x020239F8,
    .gMain = 0x03001770,
    .saveBlock1Ptr = 0x02025734,
    .saveBlock2Ptr = 0x02024EA4,
    .objectEvents = 0x030048A0,
    .mapHeader = 0x0202E828,
    .bagPockets = 0x083C1690,
    .bagPocketCount = 5,
    .encryptionKeyOff = -1,
    .bagPocketOrder = { POCKET_ITEMS, POCKET_POKE_BALLS, POCKET_TM_HM, POCKET_BERRIES, POCKET_KEY_ITEMS },
    .moneyOff = 0x490,
    
    .monIconTable = 0x083BBD98,
    .monIconPaletteIndices = 0x083BC478,
    .monIconPaletteTable = 0x083BC630,
    .itemIconTable = 0,
    
    .staticSaveBlocks = true,
    .battleStructStatic = true,
    .inBattleOff = 0x43D,
    
    .enemyParty = 0x030045C0,
    .battlerPartyIndexes = 0x02024A6A,
    .battleStructPtr = 0x02000000,
    .monToSwitchIntoOff = 0x16068,
    .language = 'E',
    .gameCode = "AXPE"
};

#endif // PD_CONFIG_H
