# 3DS PokeDaisy

**3DS PokeDaisy** is a native Nintendo 3DS port of the popular PokeDaisy companion app. Originally built as an Android app overlay for RetroArch, this project integrates PokeDaisy directly into a custom fork of the **mGBA** emulator for the 3DS, utilizing the dual-screen hardware to display real-time game data on the bottom touch screen.

By reading and decrypting the GBA's RAM in real-time, PokeDaisy tracks your Pokémon's hidden stats, inventory, Pokedex, and location, providing a seamless "second screen" experience similar to the DS-era Pokémon games.

## Download & Install

Open **FBI** on your 3DS, select **Remote Install -> Scan QR Code**, and scan the QR code below to instantly download and install the latest release directly over WiFi!

<img src="https://api.qrserver.com/v1/create-qr-code/?size=250x250&data=https://tinyurl.com/23tbyjue" width="250" height="250" alt="PokeDaisy Latest Release QR Code">

*(Alternatively, you can manually download the `.cia` file from the [Releases](https://github.com/cantpauseitmum/3DS-PokeDaisy/releases/latest) page.)*

## Features

The bottom screen features an interactive Tab Bar with the following screens:

1. **PARTY**: Displays your current 6-Pokémon roster. Tapping a Pokémon reveals its exact **IVs, EVs, Nature, Friendship, and Type Weaknesses/Resistances**, decrypting the Gen 3 data structures on the fly.
2. **BAG**: Displays your full inventory (Items, Key Items, Poké Balls, TM/HMs, Berries) organized by pocket. Includes touch-screen scrolling for large pockets.
3. **MAP**: Parses the `LZ77` compressed Map graphics directly from the ROM, drawing the region map on the bottom screen and displaying your current Location ID by reading the active map header.
4. **DEX**: Tracks your Pokedex progress (Seen/Caught) by reading the `SaveBlock1` and `SaveBlock2` bitflags. It dynamically supports both Regional and National dex configurations.
5. **CARD**: Translates the proprietary `Gen3Text` encoding to display your Trainer Name, ID Number, Playtime, and exact Money.
6. **GUIDE**: An offline walkthrough reader. Drop a `poke_guide.txt` file on the root of your 3DS SD card, and read it in real-time while you play.

### Emulator Enhancements
- **Battle Automation**: The touch screen provides massive "FIGHT" and "SWITCH" buttons during battle. PokeDaisy injects A/B/D-Pad hardware macros directly into the emulator's polling loop, automating tedious menu navigation.
- **Smart Fast-Forward**: Mapped to the `ZL` button, fast-forwarding automatically disables itself when entering battles or completing scripts.

## Supported Games

PokeDaisy uses signature-based RAM layouts (`NativeConfig`) to support the following retail games and ROM hacks:

- **Pokémon FireRed** (Rev 0 and Rev 1 - `BPRE`)
- **Pokémon LeafGreen** (Rev 0 and Rev 1 - `BPGE`)
- **Pokémon Emerald** (`BPEE`)
- **Pokémon Ruby** (`AXVE`)
- **Pokémon Sapphire** (`AXPE`)
- **Pokémon Radical Red v4.1** (Dynamically detected via the 32MB ROM expansion, shifting the Pokedex tracking flags to its custom CFRU layout up to 1025 species).

## Architecture

The project is built entirely in C within `src/platform/3ds/pokedaisy/`.
- `pd_gen3.c`: Handles the complex cryptography (XOR keys, substructure shuffling) required to read Gen 3 Pokémon data and save blocks.
- `pd_ui.c`: Manages the `citro2d` UI rendering on the 3DS bottom screen, touch-screen logic, tab switching, and mGBA core hooks.
- `pd_lz77.c`: A custom decompressor used to extract the 256x256 region map graphics directly from the game's ROM address space.

## How to Build

Building the project requires the **devkitARM** toolchain.

1. **Compile the `mgba.elf` file:**
   Using Docker, run the following command from the root of the project to compile the 3DS build:
   ```bash
   docker run --rm -v "$(pwd):/src" -w /src/build-3ds devkitpro/devkitarm make -j4
   ```

2. **Package the `.cia`:**
   Because the Docker container lacks `makerom` and `bannertool`, you must package the resulting `.elf` file natively on your host machine.
   ```bash
   # Download makerom and bannertool for your OS, then run:
   ./makerom -f cia -o PokeDaisy.cia -target t -exefslogo -i build-3ds/mgba.elf.elf:0:0
   ```

3. **Install:**
   Copy `PokeDaisy.cia` to your 3DS SD card and install it using FBI.

## Acknowledgements

This project is built upon the incredible work of two open-source projects:
- [**mGBA**](https://github.com/mgba-emu/mgba): The core GBA emulation is powered by mGBA.
- [**PokeDaisy**](https://github.com/lidor30/pokedaisy): The original companion app for Android handhelds, which inspired this native 3DS port.
