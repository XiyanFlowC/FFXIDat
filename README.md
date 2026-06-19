# FFXIDat

A comprehensive toolset for extracting, editing, and rebuilding DAT files used in Final Fantasy XI.

## Overview

This project provides tools to work with FFXI's game data files, including text strings, item data, status effects, menu textures, and more. It consists of multiple components:

- **FFXIDat**: Core library — file format parsers (DMsg, ItemData, EventStringBase, ZoneEventImage, ZoneActor)
- **FFXIDatProcessor**: CLI tool with SQLite database for batch extraction, CSV conversion, text import/export, and event index management
- **FFXIDatAdv**: Event dialogue extraction tool — structured JSON/TXT output from evev/evac/evsb
- **FFXITrans**: Translation injection tool with interactive prompts and multi-language support
- **FFXITransAdv**: Event-aware translation processor using FFXIDatAdv analysis engine
- **FFXIMenu**: GUI editor for menu textures and UI elements
- **xybase**: Utility library for string conversion and data handling

## Supported File Formats

### Text and String Data
- **XISTRING** (magic: `XISTRING`) — System messages
- **DMsg** (magic: `d_msg`) — Dialog messages and menu text
- **EventStringBase** (evsb) — Per-area event text strings
- **Fixed Phrase** — Auto-translate dictionary entries

### Event Data (FFXIDat, clean-room)
- **ZoneEventImage** (evev) — Actor blocks, constants (imed), event descriptors. Does NOT parse opcodes.
- **ZoneActor** (evac) — Entity catalog: actor_id → name, with write-back support.

### Game Data
- **Item Data** — Equipment, weapons, usables, currency, etc.
- **Status Data** — Status effects and buffs
- **Records of Eminence** — Quest and category data
- **Monster Bridge** — Monster name data

### Menu and UI
- **Block Files** (magic: `menu`) — Menu layouts and textures (DXT1/DXT3 compression)

## Quick Start

### Extraction

```bash
# Initialize SQLite database and import game DAT texts
FFXIDatProcessor.exe --sql-init
FFXIDatProcessor.exe --sql-cond-type evsb --sql-cond-lang jp --sql-dat-read

# Export event dialogue database
FFXIDatAdv.exe --dump-db --lang jp
FFXIDatAdv.exe --dump-event-json --out ./event_json
```

### Translation Workflow

```bash
# FFXITrans — interactive mode with InSitu prompts
FFXITrans.exe

# FFXITransAdv — advanced event-aware processing
FFXITransAdv.exe extract [zone]
FFXITransAdv.exe apply [zone]
```

## Building

Requires:
- Visual Studio 2022 or later
- C++20 support
- SQLite 3.48.0 (embedded as amalgamation)

Open `FFXIDat.sln` and build the solution.

## Dependencies

- SQLite 3.48.0 (amalgamation, compiled directly)

## References

- [POLUtils](https://github.com/Windower/POLUtils) — Apache 2.0 licensed FFXI DAT utilities
- [XiEvents](https://github.com/atom0s/XiEvents) — FFXI event system reverse-engineering docs

## License

See LICENSE.txt for details.
