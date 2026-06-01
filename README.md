# FFXIDat

A comprehensive toolset for extracting, editing, and rebuilding DAT files used in Final Fantasy XI.

## Overview

This project provides tools to work with FFXI's game data files, including text strings, item data, status effects, menu textures, and more. It consists of multiple components:

- **FFXIDat**: Core library for reading and writing DAT file formats
- **FFXIDatProcessor**: Command-line tool for batch extraction and database management
- **FFXITrans**: Translation injection tool for localization projects
- **FFXIMenu**: GUI editor for menu textures and UI elements
- **xybase**: Utility library for string conversion and data handling

## Supported File Formats

### Text and String Data
- **XISTRING** (magic: `XISTRING`) - System messages
	- All observed control sequence is handled, but not fully documented yet.
- **DMsg** (magic: `d_msg`) - System messages and menu text
- **Event Strings** - Per-area dialogue and event text (e.g., ROM/22/17.DAT)
	- Basic string handling is done, the control sequences are not fully documented yet
- **Fixed Phrase** - Auto-translate dictionary entries

### Game Data
- **Item Data** - Equipment, weapons, usables, currency, etc.
- **Status Data** - Status effects and buffs
- **Records of Eminence** - Quest and category data
- **Monster Bridge** - Monster name data

### Menu and UI
- **Block Files** (magic: `menu`) - Menu layouts and textures
  - Image blocks (DXT1, DXT3 texture compression)
  - Image set blocks (texture clipping and tiling)
  - Menu layout blocks

## Quick Start

### Extraction

Extract all known text files from game directory:
```bash
FFXIDatProcessor.exe --scan-extract
```

Extract specific file types:
```bash
FFXIDatProcessor.exe --dmsg-to-csv path/to/file.DAT
FFXIDatProcessor.exe --xis-to-csv path/to/file.DAT
```

### Editing

Most files can be converted to CSV for editing, then converted back:
```bash
# Edit the CSV file with your changes
FFXIDatProcessor.exe --csv-to-dmsg edited_file.csv
```

### Translation Workflow

FFXITrans provides automated translation injection:
```bash
# Interactive mode
FFXITrans.exe

# In-place modification
FFXITrans.exe insitu
```

## Building

Requires:
- Visual Studio 2022 or later
- C++20 support
- SQLite 3.48.0

Open `FFXIDat.sln` and build the solution.

## Documentation

- [File Format Specifications](docs/FILE_FORMATS.md)
- [Tool Usage Guide](docs/TOOLS.md)

## Notes

- FFXI uses extended Shift-JIS encoding to represent French and German characters
- Some file formats are partially documented
- Always backup original files before modification

## Dependencies

- SQLite 3.48.0

## License

See LICENSE.txt for details.
