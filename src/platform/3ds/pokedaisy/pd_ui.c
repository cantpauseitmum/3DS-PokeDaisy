#include "include/pd_ui.h"
#include "include/pd_config.h"
#include "include/pd_gen3.h"
#include "include/pd_data.h"
#include <mgba/core/core.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/pd_lz77.h"

static C2D_TextBuf g_staticBuf;

static const NativeConfig* g_config = NULL;
static char g_activeGameCode[5] = "";
static uint8_t g_activeRevision = 0xFF;

extern bool frameLimiter;
static bool g_pd_fastForwarding = false;
static uint32_t g_pd_lastKeysDown = 0;

uint16_t g_pd_injectedKeys = 0;
bool g_pd_injectingKeys = false;
int g_pd_injectTimer = 0;
int g_pd_injectSequence = 0;
int g_pd_injectStep = 0;

typedef struct {
    C3D_Tex tex;
    Tex3DS_SubTexture subtex;
    C2D_Image image;
    int currentSpecies;
} MonIconState;

// Sprite Rendering State
static MonIconState g_playerIcons[6];
static MonIconState g_enemyIcon;
static uint32_t* g_rgbaBuffer;
static int g_currentTab = 0; // 0: PARTY/BATTLE, 1: BAG, 2: MAP
static int g_selectedPartyIdx = 0;
static int g_bagPocketIdx = 0;
static int g_bagScroll = 0;

static const char* pd_nature_names[25] = {
    "Hardy", "Lonely", "Brave", "Adamant", "Naughty",
    "Bold", "Docile", "Relaxed", "Impish", "Lax",
    "Timid", "Hasty", "Serious", "Jolly", "Naive",
    "Modest", "Mild", "Quiet", "Bashful", "Rash",
    "Calm", "Gentile", "Sassy", "Careful", "Quirky"
};

typedef struct {
    C3D_Tex tex;
    Tex3DS_SubTexture subtex;
    C2D_Image image;
    bool loaded;
} MapState;

static MapState g_mapState;

static void InitIconState(MonIconState* state) {
    C3D_TexInit(&state->tex, 32, 32, GPU_RGBA8);
    C3D_TexSetFilter(&state->tex, GPU_LINEAR, GPU_NEAREST);
    
    state->subtex = (Tex3DS_SubTexture){
        .width = 32, .height = 32,
        .left = 0.0f, .top = 1.0f, .right = 1.0f, .bottom = 0.0f
    };
    
    state->image.tex = &state->tex;
    state->image.subtex = &state->subtex;
    state->currentSpecies = -1;
}

void PokeDaisy_InitUI(void) {
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    g_staticBuf = C2D_TextBufNew(4096);
    g_rgbaBuffer = (uint32_t*)linearMemAlign(256 * 256 * sizeof(uint32_t), 0x80);
    
    for (int i = 0; i < 6; i++) {
        InitIconState(&g_playerIcons[i]);
    }
    InitIconState(&g_enemyIcon);
    
    C3D_TexInit(&g_mapState.tex, 256, 256, GPU_RGBA8);
    C3D_TexSetFilter(&g_mapState.tex, GPU_LINEAR, GPU_NEAREST);
    g_mapState.subtex = (Tex3DS_SubTexture){
        .width = 240, .height = 160,
        .left = 0.0f, .top = 160.0f/256.0f, .right = 240.0f/256.0f, .bottom = 0.0f
    };
    g_mapState.image.tex = &g_mapState.tex;
    g_mapState.image.subtex = &g_mapState.subtex;
    g_mapState.loaded = false;
}

void PokeDaisy_CleanupUI(void) {
    for (int i = 0; i < 6; i++) {
        C3D_TexDelete(&g_playerIcons[i].tex);
    }
    C3D_TexDelete(&g_enemyIcon.tex);
    C3D_TexDelete(&g_mapState.tex);
    if (g_rgbaBuffer) linearFree(g_rgbaBuffer);
    C2D_TextBufDelete(g_staticBuf);
    C2D_Fini();
}

static void PokeDaisy_LoadMonIcon(struct mCore* core, int species, MonIconState* state) {
    if (!core || !core->busRead32) return;

    uint32_t tilesPtr = core->busRead32(core, g_config->monIconTable + (species * 4));
    uint8_t palIdx = core->busRead8(core, g_config->monIconPaletteIndices + species);
    uint32_t palDataPtr = core->busRead32(core, g_config->monIconPaletteTable + (palIdx * 8));

    uint8_t tiles[1024];
    for (int i = 0; i < 1024; i += 4) {
        uint32_t word = core->busRead32(core, tilesPtr + i);
        memcpy(&tiles[i], &word, 4);
    }

    uint16_t pal[16];
    for (int i = 0; i < 32; i += 4) {
        uint32_t word = core->busRead32(core, palDataPtr + i);
        memcpy(&pal[i / 2], &word, 4);
    }

    memset(g_rgbaBuffer, 0, 32 * 32 * 4);
    int tile = 0;
    for (int ty = 0; ty < 4; ty++) {
        for (int tx = 0; tx < 4; tx++) {
            int base = tile * 32;
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    uint8_t byteVal = tiles[base + (row * 4) + (col / 2)];
                    uint8_t idx = (col % 2 == 0) ? (byteVal & 0x0F) : (byteVal >> 4);
                    
                    uint32_t color = 0;
                    if (idx != 0) {
                        uint16_t c = pal[idx];
                        int r = (c & 0x1F) << 3;
                        int g = ((c >> 5) & 0x1F) << 3;
                        int b = ((c >> 10) & 0x1F) << 3;
                        r |= (r >> 5); g |= (g >> 5); b |= (b >> 5);
                        color = r | (g << 8) | (b << 16) | (255 << 24);
                    }
                    
                    int px_y = ty * 8 + row;
                    int px_x = tx * 8 + col;
                    g_rgbaBuffer[px_y * 32 + px_x] = color;
                }
            }
            tile++;
        }
    }
    
    GSPGPU_FlushDataCache(g_rgbaBuffer, 32 * 32 * 4);
    C3D_SyncDisplayTransfer(
        (uint32_t*)g_rgbaBuffer, GX_BUFFER_DIM(32, 32),
        (uint32_t*)state->tex.data, GX_BUFFER_DIM(32, 32),
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_FLIP_VERT(1)
    );
}


static Gen3PartyMon ReadPartyMon(struct mCore* core, uint32_t partyBaseAddr, int index) {
    uint8_t raw[MON_STRUCT_SIZE];
    for (int i = 0; i < MON_STRUCT_SIZE; i += 4) {
        uint32_t word = core->busRead32(core, partyBaseAddr + (index * MON_STRUCT_SIZE) + i);
        memcpy(raw + i, &word, 4);
    }
    return PokeDaisy_DecodePartyMon(raw);
}

void PokeDaisy_DrawBottomScreen(C3D_RenderTarget* bottomScreen, struct mCore* core) {
    C2D_Prepare(); 
    C2D_SceneBegin(bottomScreen);
    C2D_TargetClear(bottomScreen, C2D_Color32(240, 240, 240, 255));
    
    C2D_TextBufClear(g_staticBuf);
    
    // Process Input
    uint32_t pKeysDown = hidKeysDown();
    uint32_t pKeysHeld = hidKeysHeld();
    
    // Prevent multiple triggers by checking if it was just pressed
    if ((pKeysDown & KEY_X) && !(g_pd_lastKeysDown & KEY_X)) {
        if (core && core->saveState) mCoreSaveState(core, 1, 0);
    }
    if ((pKeysDown & KEY_Y) && !(g_pd_lastKeysDown & KEY_Y)) {
        if (core && core->loadState) mCoreLoadState(core, 1, 0);
    }
    if ((pKeysDown & KEY_ZL) && !(g_pd_lastKeysDown & KEY_ZL)) {
        g_pd_fastForwarding = !g_pd_fastForwarding;
    }
    
    if (pKeysDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        
        // Tab Bar
        if (touch.py > 210) { 
            if (touch.px < 53) g_currentTab = 0;
            else if (touch.px < 106) g_currentTab = 1;
            else if (touch.px < 159) g_currentTab = 2;
            else if (touch.px < 212) g_currentTab = 3;
            else if (touch.px < 265) g_currentTab = 4;
            else g_currentTab = 5;
        }
        
        // Touch Fast-Forward Toggle (Top Right)
        if (touch.py < 30 && touch.px > 270) {
            if (!(g_pd_lastKeysDown & KEY_TOUCH)) {
                g_pd_fastForwarding = !g_pd_fastForwarding;
            }
        }
        
        // Touch Roster Selector (Left panel, Party tab)
        if (g_currentTab == 0 && touch.px < 120 && touch.py < 180) {
            g_selectedPartyIdx = touch.py / 30;
            if (g_selectedPartyIdx > 5) g_selectedPartyIdx = 5;
        }
        
        // Touch Bag Pocket Selector
        if (g_currentTab == 1) {
            if (touch.py < 30) {
                if (touch.px < 60) g_bagPocketIdx = 0;
                else if (touch.px < 120) g_bagPocketIdx = 1;
                else if (touch.px < 180) g_bagPocketIdx = 2;
                else if (touch.px < 240) g_bagPocketIdx = 3;
                else g_bagPocketIdx = 4;
                g_bagScroll = 0;
            } else if (touch.px > 260) {
                // Scroll Buttons
                if (touch.py < 100 && g_bagScroll > 0) g_bagScroll--;
                else if (touch.py > 120 && touch.py < 200) g_bagScroll++;
            }
        }
    }
    g_pd_lastKeysDown = pKeysDown;
    
    bool inBattle = false;
    
    if (core) {
        size_t romSize = 0;
        uint8_t* rom = core->getMemoryBlock(core, 0x08000000, &romSize);
        if (rom && romSize >= 0x100) {
            char gameCode[5];
            memcpy(gameCode, rom + 0xAC, 4);
            gameCode[4] = '\0';
            uint8_t revision = rom[0xBC];
            
            if (strcmp(gameCode, g_activeGameCode) != 0 || revision != g_activeRevision) {
                strcpy(g_activeGameCode, gameCode);
                g_activeRevision = revision;
                
                if (strcmp(gameCode, "BPRE") == 0) {
                    if (romSize >= 0x2000000) {
                        g_config = &NATIVE_RADICAL_RED;
                    } else {
                        if (revision == 0) g_config = &NATIVE_FIRERED_REV0;
                        else g_config = &NATIVE_FIRERED_REV1;
                    }
                } else if (strcmp(gameCode, "BPGE") == 0) {
                    if (revision == 0) g_config = &NATIVE_LEAFGREEN_REV0;
                    else g_config = &NATIVE_LEAFGREEN_REV1;
                } else if (strcmp(gameCode, "BPEE") == 0) {
                    g_config = &NATIVE_EMERALD;
                } else if (strcmp(gameCode, "AXVE") == 0) {
                    g_config = &NATIVE_RUBY;
                } else if (strcmp(gameCode, "AXPE") == 0) {
                    g_config = &NATIVE_SAPPHIRE;
                } else {
                    g_config = NULL; // Unsupported game
                }
                
                g_mapState.loaded = false; // Force map reload
            }
        }
        
        // Smart Fast-Forward Check
        if (core->busRead32 && g_config) {
            uint32_t gMain = core->busRead32(core, g_config->gMain);
            if (gMain) {
                uint8_t callbackIdx = core->busRead8(core, gMain + g_config->inBattleOff);
                inBattle = (callbackIdx != 0);
            }
        }
    }
    
    // Run BattleInputSequencer State Machine
    if (g_pd_injectingKeys) {
        g_pd_injectTimer--;
        if (g_pd_injectTimer <= 0) {
            g_pd_injectStep++;
            g_pd_injectedKeys = 0;
            
            if (g_pd_injectSequence == 1) { // FIGHT (A)
                if (g_pd_injectStep == 1) { g_pd_injectedKeys = 1; g_pd_injectTimer = 3; }
                else { g_pd_injectingKeys = false; }
            } else if (g_pd_injectSequence == 2) { // SWITCH (Down, Right, A)
                if (g_pd_injectStep == 1) { g_pd_injectedKeys = 128; g_pd_injectTimer = 3; } // Down
                else if (g_pd_injectStep == 2) { g_pd_injectedKeys = 0; g_pd_injectTimer = 3; }
                else if (g_pd_injectStep == 3) { g_pd_injectedKeys = 16; g_pd_injectTimer = 3; } // Right
                else if (g_pd_injectStep == 4) { g_pd_injectedKeys = 0; g_pd_injectTimer = 3; }
                else if (g_pd_injectStep == 5) { g_pd_injectedKeys = 1; g_pd_injectTimer = 3; } // A
                else { g_pd_injectingKeys = false; }
            }
        }
    }
    
    // Apply Frame Limiter
    bool smartFF = (pKeysHeld & KEY_ZR) && inBattle;
    frameLimiter = !(g_pd_fastForwarding || smartFF);
    
    if (core && core->busRead32 && g_config) {
        if (g_currentTab == 0) {
            uint8_t partyCount = core->busRead8(core, g_config->playerPartyCount);
            if (partyCount > 6) partyCount = 6;
            
            // Draw 6-Slot Roster on the left
            for (int i = 0; i < 6; i++) {
                Gen3PartyMon mon = ReadPartyMon(core, g_config->playerParty, i);
                float yOff = i * 30.0f;
                
                if (i == g_selectedPartyIdx) {
                    C2D_DrawRectSolid(0, yOff, 0.5f, 130, 30, C2D_Color32(100, 100, 100, 255));
                }
                
                if (mon.isValid && i < partyCount) {
                    if (g_playerIcons[i].currentSpecies != mon.species) {
                        PokeDaisy_LoadMonIcon(core, mon.species, &g_playerIcons[i]);
                        g_playerIcons[i].currentSpecies = mon.species;
                    }
                    C2D_DrawImageAt(g_playerIcons[i].image, 2.0f, yOff + 2.0f, 0.5f, NULL, 0.8f, 0.8f);
                    
                    float hp_pct = 0;
                    if (mon.maxHp > 0) hp_pct = (float)mon.hp / (float)mon.maxHp;
                    uint32_t c = (hp_pct > 0.5f) ? C2D_Color32(72, 208, 112, 255) : 
                                 (hp_pct > 0.2f) ? C2D_Color32(248, 208, 0, 255) : C2D_Color32(248, 88, 56, 255);
                    C2D_DrawRectSolid(32.0f, yOff + 12.0f, 0.5f, 80.0f, 6.0f, C2D_Color32(64, 64, 64, 255));
                    if (hp_pct > 0) C2D_DrawRectSolid(32.0f, yOff + 12.0f, 0.5f, 80.0f * hp_pct, 6.0f, c);
                    
                    char lvlStr[32];
                    snprintf(lvlStr, sizeof(lvlStr), "Lv.%d", mon.level);
                    C2D_Text t;
                    C2D_TextParse(&t, g_staticBuf, lvlStr);
                    C2D_TextOptimize(&t);
                    C2D_DrawText(&t, C2D_WithColor, 32.0f, yOff + 20.0f, 0.5f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));
                } else {
                    g_playerIcons[i].currentSpecies = -1;
                }
            }
            
            // Draw Advanced Stats for selected Mon on the right
            Gen3PartyMon selMon = ReadPartyMon(core, g_config->playerParty, g_selectedPartyIdx);
            if (selMon.isValid && g_selectedPartyIdx < partyCount) {
                const char* species = (selMon.species <= 411) ? pd_species_names[selMon.species] : "???";
                char statText[512];
                snprintf(statText, sizeof(statText), 
                         "[%s]\n"
                         "Nature: %s\n"
                         "Friendship: %d\n"
                         "HP: %d/%d (IV:%d, EV:%d)\n"
                         "Atk: %d (IV:%d, EV:%d)\n"
                         "Def: %d (IV:%d, EV:%d)\n"
                         "SpA: %d (IV:%d, EV:%d)\n"
                         "SpD: %d (IV:%d, EV:%d)\n"
                         "Spe: %d (IV:%d, EV:%d)", 
                         species,
                         pd_nature_names[selMon.nature],
                         selMon.friendship,
                         selMon.hp, selMon.maxHp, selMon.ivs[0], selMon.evs[0],
                         selMon.atk, selMon.ivs[1], selMon.evs[1],
                         selMon.def, selMon.ivs[2], selMon.evs[2],
                         selMon.spatk, selMon.ivs[4], selMon.evs[4],
                         selMon.spdef, selMon.ivs[5], selMon.evs[5],
                         selMon.speed, selMon.ivs[3], selMon.evs[3]
                );
                C2D_Text t;
                C2D_TextParse(&t, g_staticBuf, statText);
                C2D_TextOptimize(&t);
                C2D_DrawText(&t, C2D_WithColor, 140.0f, 10.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(0, 0, 0, 255));
                
                if (inBattle) {
                    // Draw Touch Battle Controls
                    C2D_DrawRectSolid(140.0f, 160.0f, 0.5f, 75.0f, 40.0f, C2D_Color32(200, 50, 50, 255));
                    C2D_Text f;
                    C2D_TextParse(&f, g_staticBuf, "FIGHT");
                    C2D_TextOptimize(&f);
                    C2D_DrawText(&f, C2D_WithColor, 150.0f, 170.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                    
                    C2D_DrawRectSolid(225.0f, 160.0f, 0.5f, 75.0f, 40.0f, C2D_Color32(50, 50, 200, 255));
                    C2D_Text s;
                    C2D_TextParse(&s, g_staticBuf, "SWITCH");
                    C2D_TextOptimize(&s);
                    C2D_DrawText(&s, C2D_WithColor, 230.0f, 170.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                    
                    // Touch logic for Battle Buttons
                    if (pKeysDown & KEY_TOUCH) {
                        touchPosition touch;
                        hidTouchRead(&touch);
                        if (touch.py >= 160 && touch.py <= 200) {
                            if (touch.px >= 140 && touch.px <= 215 && !g_pd_injectingKeys) {
                                g_pd_injectingKeys = true;
                                g_pd_injectSequence = 1; // FIGHT
                                g_pd_injectStep = 0;
                                g_pd_injectTimer = 1;
                            } else if (touch.px >= 225 && touch.px <= 300 && !g_pd_injectingKeys) {
                                g_pd_injectingKeys = true;
                                g_pd_injectSequence = 2; // SWITCH
                                g_pd_injectStep = 0;
                                g_pd_injectTimer = 1;
                            }
                        }
                    }
                }
            }
            
        } else if (g_currentTab == 1) {
            // BAG PARSING
            int pocketCap = 0;
            uint32_t pocketPtr = 0;
            
            if (g_bagPocketIdx >= g_config->bagPocketCount) g_bagPocketIdx = g_config->bagPocketCount - 1;
            
            uint32_t saveBlock2 = core->busRead32(core, g_config->saveBlock2Ptr);
            uint16_t key = 0;
            if (g_config->encryptionKeyOff >= 0) {
                key = core->busRead32(core, saveBlock2 + g_config->encryptionKeyOff) & 0xFFFF;
            }
            
            // Read specific pocket based on game's memory layout
            uint32_t basePockets = core->busRead32(core, g_config->bagPockets);
            BagPocketType pType = g_config->bagPocketOrder[g_bagPocketIdx];
            
            // Pockets are contiguous array of {ptr, cap} structs
            pocketPtr = core->busRead32(core, basePockets + (pType * 8));
            pocketCap = core->busRead8(core, basePockets + (pType * 8) + 4);
            

            
            char bag_text[512] = "";
            int drawn = 0;
            int totalValid = 0;
            
            for (int i = 0; i < pocketCap; i++) {
                uint32_t word = core->busRead32(core, pocketPtr + (i * 4));
                uint16_t id = word & 0xFFFF;
                if (id == 0) continue;
                
                if (totalValid >= g_bagScroll && drawn < 9) {
                    uint16_t qty = (word >> 16) ^ key;
                    if (pType == POCKET_KEY_ITEMS) qty = 1; // Key items usually have qty encrypted weirdly or 0
                    
                    const char* itemName = "???";
                    if (id <= 400) itemName = pd_item_names[id];
                    
                    char line[64];
                    snprintf(line, sizeof(line), "- %s x%d\n", itemName, qty);
                    strncat(bag_text, line, sizeof(bag_text) - strlen(bag_text) - 1);
                    drawn++;
                }
                totalValid++;
            }
            
            if (totalValid == 0) strncat(bag_text, "(Empty)", sizeof(bag_text) - strlen(bag_text) - 1);
            if (g_bagScroll > totalValid - 9 && totalValid >= 9) g_bagScroll = totalValid - 9;
            
            // Draw Pocket Tabs
            C2D_DrawRectSolid(0, 0, 0.5f, 320, 20, C2D_Color32(60, 60, 60, 255));
            for (int i = 0; i < 5; i++) {
                C2D_DrawRectSolid(i * 64, 0, 0.5f, 62, 20, (i == g_bagPocketIdx) ? C2D_Color32(200, 200, 200, 255) : C2D_Color32(100, 100, 100, 255));
                char pL[4]; snprintf(pL, sizeof(pL), "%d", i+1);
                C2D_Text t; C2D_TextParse(&t, g_staticBuf, pL); C2D_TextOptimize(&t);
                C2D_DrawText(&t, C2D_WithColor, i * 64 + 25, 2, 0.5f, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255));
            }
            
            // Draw Scroll buttons
            C2D_DrawRectSolid(280, 30, 0.5f, 40, 50, C2D_Color32(120, 120, 120, 255));
            C2D_DrawRectSolid(280, 150, 0.5f, 40, 50, C2D_Color32(120, 120, 120, 255));
            
            C2D_Text c2dBagText;
            C2D_TextParse(&c2dBagText, g_staticBuf, bag_text);
            C2D_TextOptimize(&c2dBagText);
            C2D_DrawText(&c2dBagText, C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 0, 0, 255));
        } else if (g_currentTab == 2) {
            // MAP ENGINE
            if (g_config->regionMapGfx != 0 && g_config->regionMapPal != 0 && g_config->regionMapTilemap != 0) {
                if (!g_mapState.loaded) {
                    size_t romSize = 0;
                    uint8_t* rom = core->getMemoryBlock(core, 0x08000000, &romSize);
                    
                    if (rom && romSize > (g_config->regionMapTilemap & 0x01FFFFFF)) {
                        uint32_t gfxAddr = g_config->regionMapGfx & 0x01FFFFFF;
                        uint32_t palAddr = g_config->regionMapPal & 0x01FFFFFF;
                        uint32_t mapAddr = g_config->regionMapTilemap & 0x01FFFFFF;
                        
                        // Need about 0x12000 for gfx, 0x800 for map
                        uint8_t* gfxData = malloc(0x12000);
                        uint8_t* mapData = malloc(0x1000);
                        
                        uint32_t gfxSize = PokeDaisy_LZ77Decompress(rom + gfxAddr, romSize - gfxAddr, gfxData);
                        uint32_t mapSize = PokeDaisy_LZ77Decompress(rom + mapAddr, romSize - mapAddr, mapData);
                        
                        if (gfxSize > 0 && mapSize >= (30 * 20 * 2)) {
                            // Extract palette (16 palettes of 16 colors)
                            uint32_t palette[16 * 16];
                            for (int i = 0; i < 16 * 16; i++) {
                                uint16_t color = rom[palAddr + i*2] | (rom[palAddr + i*2 + 1] << 8);
                                uint8_t r = (color & 0x1F) << 3;
                                uint8_t g = ((color >> 5) & 0x1F) << 3;
                                uint8_t b = ((color >> 10) & 0x1F) << 3;
                                palette[i] = (255 << 24) | (b << 16) | (g << 8) | r; // ABGR
                            }
                            
                            // Render tiles
                            for (int ty = 0; ty < 20; ty++) {
                                for (int tx = 0; tx < 30; tx++) {
                                    uint16_t tileData = mapData[(ty * 30 + tx) * 2] | (mapData[(ty * 30 + tx) * 2 + 1] << 8);
                                    int tileIdx = tileData & 0x3FF;
                                    bool flipX = (tileData & 0x400) != 0;
                                    bool flipY = (tileData & 0x800) != 0;
                                    int palBank = (tileData >> 12) & 0xF;
                                    
                                    for (int y = 0; y < 8; y++) {
                                        for (int x = 0; x < 8; x++) {
                                            int px = flipX ? 7 - x : x;
                                            int py = flipY ? 7 - y : y;
                                            
                                            uint32_t pixelOffset = (tileIdx * 32) + (py * 4) + (px / 2);
                                            if (pixelOffset < gfxSize) {
                                                uint8_t byte = gfxData[pixelOffset];
                                                uint8_t colorIdx = (px % 2 == 0) ? (byte & 0xF) : (byte >> 4);
                                                
                                                uint32_t finalColor = palette[palBank * 16 + colorIdx];
                                                
                                                // Convert 240x160 space to 256x256 texture space
                                                int drawX = tx * 8 + x;
                                                int drawY = ty * 8 + y;
                                                g_rgbaBuffer[drawY * 256 + drawX] = finalColor;
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // Upload to texture
                            GSPGPU_FlushDataCache(g_rgbaBuffer, 256 * 256 * 4);
                            C3D_SyncDisplayTransfer(
                                (uint32_t*)g_rgbaBuffer, GX_BUFFER_DIM(256, 256),
                                (uint32_t*)g_mapState.tex.data, GX_BUFFER_DIM(256, 256),
                                (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
                                GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8))
                            );
                            gspWaitForEvent(GSPGPU_EVENT_VBlank0, false);
                            g_mapState.loaded = true;
                        }
                        
                        free(gfxData);
                        free(mapData);
                    }
                }
                if (g_mapState.loaded) {
                    C2D_DrawImageAt(g_mapState.image, 40.0f, 25.0f, 0.5f, NULL, 1.0f, 1.0f);
                    
                    uint32_t mapHeaderPtr = core->busRead32(core, g_config->mapHeader);
                    if (mapHeaderPtr) {
                        uint8_t mapSecId = core->busRead8(core, mapHeaderPtr + 0x14);
                        char locText[64];
                        snprintf(locText, sizeof(locText), "Current Loc ID: %d", mapSecId);
                        C2D_Text c2dLoc;
                        C2D_TextParse(&c2dLoc, g_staticBuf, locText);
                        C2D_TextOptimize(&c2dLoc);
                        C2D_DrawRectSolid(40.0f, 185.0f, 0.5f, 240.0f, 20.0f, C2D_Color32(0, 0, 0, 180));
                        C2D_DrawText(&c2dLoc, C2D_WithColor, 45.0f, 187.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                    }
                }
            } else {
                C2D_Text noMapText;
                C2D_TextParse(&noMapText, g_staticBuf, "Map not supported for this game yet.");
                C2D_TextOptimize(&noMapText);
                C2D_DrawText(&noMapText, C2D_WithColor, 10.0f, 80.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 0, 0, 255));
            }
        } else if (g_currentTab == 3) {
            // DEX
            uint32_t saveBlock1 = core->busRead32(core, g_config->saveBlock1Ptr);
            uint32_t saveBlock2 = core->busRead32(core, g_config->saveBlock2Ptr);
            
            int seenCount = 0;
            int caughtCount = 0;
            bool national = false;
            int maxDex = 151; // default Kanto/Hoenn regional
            
            if (saveBlock1 && saveBlock2 && g_config->gameCode) {
                uint32_t seenOff = saveBlock2 + 0x5C;
                uint32_t caughtOff = saveBlock2 + 0x28;
                uint32_t seenCopy1 = 0, seenCopy2 = 0;
                
                size_t romSize = 0;
                core->getMemoryBlock(core, 0x08000000, &romSize);
                
                if (strcmp(g_config->gameCode, "BPRE") == 0 || strcmp(g_config->gameCode, "BPGE") == 0) {
                    if (romSize >= 0x2000000) { // Radical Red
                        seenOff = saveBlock1 + 0x310;
                        caughtOff = saveBlock1 + 0x3B4;
                        seenCopy1 = seenOff; seenCopy2 = seenOff; // No copies in RR
                        national = true; maxDex = 1025;
                    } else { // FireRed / LeafGreen
                        seenCopy1 = saveBlock1 + 0x5F8;
                        seenCopy2 = saveBlock1 + 0x3A18;
                        if (core->busRead8(core, saveBlock2 + 0x1B) == 0xB9) {
                            national = true; maxDex = 386;
                        }
                    }
                } else if (strcmp(g_config->gameCode, "BPEE") == 0) { // Emerald
                    seenCopy1 = saveBlock1 + 0x988;
                    seenCopy2 = saveBlock1 + 0x3B24;
                    maxDex = 202; // Hoenn dex count
                    if (core->busRead8(core, saveBlock2 + 0x1A) == 0xDA) {
                        national = true; maxDex = 386;
                    }
                } else if (strcmp(g_config->gameCode, "AXVE") == 0 || strcmp(g_config->gameCode, "AXPE") == 0) { // Ruby / Sapphire
                    seenCopy1 = saveBlock1 + 0x938;
                    seenCopy2 = saveBlock1 + 0x3A8C;
                    maxDex = 202; // Hoenn dex count
                    if (core->busRead8(core, saveBlock2 + 0x1A) == 0xDA) {
                        national = true; maxDex = 386;
                    }
                }
                
                if (seenCopy1 && seenCopy2) {
                    for (int n = 1; n <= maxDex; n++) {
                        int byteIdx = (n - 1) / 8;
                        int bitIdx = (n - 1) % 8;
                        uint8_t mask = 1 << bitIdx;
                        
                        uint8_t sBase = core->busRead8(core, seenOff + byteIdx);
                        uint8_t sC1 = core->busRead8(core, seenCopy1 + byteIdx);
                        uint8_t sC2 = core->busRead8(core, seenCopy2 + byteIdx);
                        uint8_t cBase = core->busRead8(core, caughtOff + byteIdx);
                        
                        if ((sBase & mask) && (sC1 & mask) && (sC2 & mask)) {
                            seenCount++;
                            if (cBase & mask) caughtCount++;
                        }
                    }
                }
            }
            
            char dexText[256];
            if (saveBlock1 && saveBlock2) {
                snprintf(dexText, sizeof(dexText), "[POKEDEX - %s]\n\nSeen: %d\nCaught: %d\n\nTotal: %d", 
                         national ? "National" : "Regional", seenCount, caughtCount, maxDex);
            } else {
                snprintf(dexText, sizeof(dexText), "Pokedex not available yet.");
            }
            
            C2D_Text t;
            C2D_TextParse(&t, g_staticBuf, dexText);
            C2D_TextOptimize(&t);
            C2D_DrawText(&t, C2D_WithColor, 10.0f, 50.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 0, 0, 255));
        } else if (g_currentTab == 4) {
            // CARD
            uint32_t saveBlock2 = core->busRead32(core, g_config->saveBlock2Ptr);
            uint32_t money = 0;
            char trainerName[8] = "???";
            uint16_t trainerId = 0;
            uint16_t hours = 0;
            uint8_t minutes = 0;
            
            if (saveBlock2) {
                // Decode Gen3 string
                for (int i=0; i<7; i++) {
                    uint8_t b = core->busRead8(core, saveBlock2 + i);
                    if (b == 0xFF) { trainerName[i] = '\0'; break; }
                    if (b >= 0xBB && b <= 0xD4) trainerName[i] = 'A' + (b - 0xBB);
                    else if (b >= 0xD5 && b <= 0xEE) trainerName[i] = 'a' + (b - 0xD5);
                    else if (b >= 0xA1 && b <= 0xAA) trainerName[i] = '0' + (b - 0xA1);
                    else if (b == 0x00) trainerName[i] = ' ';
                    else trainerName[i] = '?';
                }
                trainerName[7] = '\0';
                
                trainerId = core->busRead16(core, saveBlock2 + 0x0A);
                hours = core->busRead16(core, saveBlock2 + 0x0E);
                minutes = core->busRead8(core, saveBlock2 + 0x10);
                
                if (g_config->moneyOff >= 0) {
                    uint16_t key = 0;
                    if (g_config->encryptionKeyOff >= 0) {
                        key = core->busRead32(core, saveBlock2 + g_config->encryptionKeyOff) & 0xFFFF;
                    }
                    uint32_t mWord = core->busRead32(core, saveBlock2 + g_config->moneyOff);
                    money = mWord ^ (key | (key << 16));
                }
            }
            
            char cardText[256];
            snprintf(cardText, sizeof(cardText), "[Trainer Card]\n\nName: %s\nIDNo. %05d\n\nMoney: $ %u\nTime: %d:%02d", 
                     trainerName, trainerId, money, hours, minutes);
            C2D_Text t;
            C2D_TextParse(&t, g_staticBuf, cardText);
            C2D_TextOptimize(&t);
            C2D_DrawText(&t, C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 0, 0, 255));
        } else if (g_currentTab == 5) {
            // GUIDE
            FILE* f = fopen("sdmc:/poke_guide.txt", "r");
            char guideText[512] = "No guide found.\nPlease place poke_guide.txt\non the root of your SD card.";
            if (f) {
                fread(guideText, 1, 511, f);
                guideText[511] = '\0';
                fclose(f);
            }
            C2D_Text t;
            C2D_TextParse(&t, g_staticBuf, guideText);
            C2D_TextOptimize(&t);
            C2D_DrawText(&t, C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(0, 0, 0, 255));
        }
    } else if (core && core->busRead32 && !g_config) {
        C2D_Text noConfigText;
        C2D_TextParse(&noConfigText, g_staticBuf, "Game not supported yet.");
        C2D_TextOptimize(&noConfigText);
        C2D_DrawText(&noConfigText, C2D_WithColor, 10.0f, 80.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 255));
    }
    
    // Draw Tab Bar
    C2D_DrawRectSolid(0, 210, 0, 320, 30, C2D_Color32(200, 200, 200, 255));
    C2D_DrawRectSolid(g_currentTab * 53, 210, 0, 53, 30, C2D_Color32(255, 255, 255, 255));
    
    const char* tabs[6] = {"PARTY", "BAG", "MAP", "DEX", "CARD", "GUIDE"};
    for (int i = 0; i < 6; i++) {
        C2D_Text t;
        C2D_TextParse(&t, g_staticBuf, tabs[i]);
        C2D_TextOptimize(&t);
        C2D_DrawText(&t, C2D_WithColor, i * 53 + 5.0f, 215.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(0, 0, 0, 255));
    }
    
    // Draw Touch FF Button
    C2D_DrawRectSolid(270, 0, 0.5f, 50, 30, g_pd_fastForwarding ? C2D_Color32(0, 255, 0, 255) : C2D_Color32(80, 80, 80, 255));
    C2D_Text ffText;
    C2D_TextParse(&ffText, g_staticBuf, "FF");
    C2D_TextOptimize(&ffText);
    C2D_DrawText(&ffText, C2D_WithColor, 285.0f, 5.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
    
    C2D_Flush(); 
}
