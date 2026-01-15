# FFXI DAT File Format Specifications

This document describes the binary format of various DAT files used in Final Fantasy XI.

## Table of Contents

- [Text and String Formats](#text-and-string-formats)
  - [XISTRING](#xistring)
  - [DMsg](#dmsg)
  - [Event Strings](#event-strings)
  - [Fixed Phrase](#fixed-phrase)
- [Game Data Formats](#game-data-formats)
  - [Item Data](#item-data)
  - [Status Data](#status-data)
  - [Records of Eminence](#records-of-eminence)
  - [Monster Bridge](#monster-bridge)
- [Menu and UI Formats](#menu-and-ui-formats)
  - [Block Files](#block-files)

---

## Text and String Formats

### XISTRING

**Magic Header**: `XISTRING` (8 bytes ASCII)

**Purpose**: System messages, UI strings

#### File Structure

```
[Header] [Index Array] [String Data]
```

#### Header Format

```cpp
struct XiStringHeader {
    char magicHeader[8];      // "XISTRING"
    int32_t version;          // Always 0x20000
    int32_t zero[5];          // Always 0
    int32_t fileSize;         // Total file size
    int32_t entriesCount;     // Number of strings
    int32_t indicesSize;      // Size of index array
    int32_t dataSize;         // Size of string data
    int32_t reserved;         // Always 0
    int32_t id;               // File identifier
};
```

#### Index Entry Format

```cpp
struct XiStringIndex {
    int32_t offset;    // Offset relative to start of string data
    uint16_t size;     // String length
    uint16_t flag1;    // Unknown
    uint16_t flag2;    // Unknown
    uint16_t flag3;    // Unknown
};
```

**Notes**:
- All offsets are relative to the beginning of the string data block
- Strings are stored in Shift-JIS encoding
- Index array immediately follows the header

---

### DMsg

**Magic Header**: `d_msg` (5 bytes ASCII + padding)

**Purpose**: System messages, menu text, dialogue options

#### File Structure

```
[Header] [Optional Index] [Row 1] [Row 2] ... [Row N]
```

#### Header Format

```cpp
struct DMsgHeader {
    char magic[8];           // "d_msg" + padding
    uint32_t headerSize;     // Size of header section
    uint32_t entryCount;     // Number of rows
    uint32_t dataSize;       // Total data size
    uint8_t hasIndex;        // 1 if index present, 0 otherwise
    // Additional fields vary by file
};
```

#### Row Format

```
[Cell Count][Cell1 Metadata][Cell2 Metadata]...[Cell1 Data][Cell2 Data]...
```

```cpp
struct RecordSpec {
    int32_t offset;    // Offset to cell data
    int32_t type;      // 0 = string, 1 = integer
};

struct Record {
    int32_t cellCount;
    RecordSpec spec[cellCount];
    // Followed by cell data
};
```

**Cell Types**:
- Type 0 (String): Null-terminated Shift-JIS string
- Type 1 (Integer): 32-bit signed integer

**Variations**:
- **Block Mode**: Fixed-size rows with padding
- **Variable Mode**: Variable-size rows, tightly packed
- **XOR Obfuscation**: Some files use XOR with 0xFF for obfuscation (see the `obs` field in the header and `DMsg::Xor` in [`FFXIDat/DMsg.h`](../FFXIDat/DMsg.h)).

---

### Event Strings

**Magic Pattern**: The first 4 bytes encode the file size (lower 24 bits) and a flag (upper 8 bits, usually 0x10).

**Purpose**: Area-specific dialogue, event text, NPC speech.

#### File Structure

```
[Header (4 bytes)] [Offset Table] [String Data] [Terminator]
```

##### Header
- 4 bytes: lower 24 bits are the size of the rest of the file, upper 8 bits are a flag (usually 0x10).
- See `EventStringBaseHeader` in [`FFXIDat/EventStringBase.h`](../FFXIDat/EventStringBase.h).

##### Offset Table
- Array of 32-bit integers, each giving the offset (from the start of the offset table) to a string.
- The number of offsets equals the number of strings.
- The offset table is immediately followed by the string data.
- All offsets and string data are XOR-obfuscated with 0x80 if the flag is set (see `EventStringBase::Xor`).

##### String Data
- Strings are encoded in Shift-JIS and may contain control codes.
- Each string is located at the offset specified in the offset table.
- The last string is followed by a terminator byte: 0x80 if obfuscated, 0x00 otherwise.

##### Notes
- All offsets are relative to the start of the offset table, not the file.
- The file is not encrypted, only XOR-obfuscated if the flag is set.
- The code for reading and writing is in [`FFXIDat/EventStringBase.cpp`](../FFXIDat/EventStringBase.cpp).

##### Control Sequences


Event string control codes are single-byte binary codes (not `\x1F`-prefixed tags) that may be followed by zero or more parameters. The mapping between binary codes and their textual representation is defined in the code (see `gameStringControlSequenceDefinition` in [`FFXIDat/EventString.h`](../FFXIDat/EventString.h)).

**Encoding/Decoding:**
- The `EventStringCodecUtil` class provides methods to encode and decode these control codes between their binary form and a human-readable tag (e.g., `<item:12:34>`).
- When decoding, binary codes are mapped to tag names and parameters; when encoding, tags are converted back to binary codes and parameters.

**Control Code Table (Partial):**
| Type Name | Code (hex) | Parameters | Description |
|-----------|------------|------------|-------------|
| ins       | 01         | 1          | Special proc, type byte follows |
| 02        | 02         | 0          | Unknown |
| 03        | 03         | 0          | Unknown |
| 04        | 04         | 0          | Unknown |
| 05        | 05         | 1          | Unknown |
| lf        | 07         | 0          | Line feed |
| name      | 08         | 0          | Player or character name |
| num       | 0A         | 1          | Number insertion |
| sel       | 0B         | 0          | Selection start |
| switch    | 0C         | 1          | Switch/branch |
| magic     | 10         | 1          | Magic name insertion |
| faith     | 11         | 1          | Faith/magic type |
| int       | 12         | 1          | Integer insertion |
| item      | 13         | 1          | Item name insertion |
| ws        | 16         | 1          | Weapon skill |
| time      | 18         | 1          | Time value |
| weather   | 1A         | 1          | Weather name |
| str       | 1C         | 1          | String insertion |
| color2    | 1E         | 1          | Colour change (variant) |
| color     | 1F         | 1          | Colour change |
| val       | EF         | 1          | Text value insertion |

**Format:**
- Each control code is a single byte, optionally followed by the specified number of parameter bytes.
- Example: `<item:0>` encodes as `\x13\x00`.
- The codec utility recognises and translates these codes using its internal mapping.

**Notes:**
- Not all codes are fully documented; some are game-specific and may require further reverse engineering.
- Codes are not printable and are embedded directly in the Shift-JIS string data.
- The codec utility ensures round-trip fidelity between binary and tag forms.

**References:**
- [`FFXIDat/EventString.h`](../FFXIDat/EventString.h): Control sequence definitions and codec utility
- [`FFXIDat/EventStringBase.cpp`](../FFXIDat/EventStringBase.cpp): String reading/writing logic

##### References
- [`FFXIDat/EventStringBase.h`](../FFXIDat/EventStringBase.h): Structure definition
- [`FFXIDat/EventStringBase.cpp`](../FFXIDat/EventStringBase.cpp): Read/write logic and obfuscation

---

### Fixed Phrase

**Magic Pattern**: Starts with `0x02 0x01` or `0x02 0x02` (heuristic, not a fixed header, the provided patterns are actually part of category/entry headers)

**Purpose**: Auto-translate dictionary, preset phrases

#### File Structure

```
[Category 1][Category 2]...[Category N]
```

#### Category Header

```cpp
struct fixed_phrase_category {
    uint8_t a;        // 0x02
    uint8_t b;        // 0x01
    uint8_t cat;      // Category index
    uint8_t ent;      // Entry index
};

struct fixed_phrase_category_header {
    fixed_phrase_category cat;
    char cat_name[32];     // Category name (Shift-JIS)
    char cat_pron[32];     // Pronunciation
    int32_t count;         // Number of entries
    int32_t size;          // Category data size
};
```

#### Entry Format

Each entry consists of:
- `fixed_phrase_category` header
- Null-terminated text string
- Null-terminated pronunciation string

---

## Game Data Formats

### Item Data

**Location**: ROM/0/4 through ROM/0/9, ROM/286/72

**Purpose**: Stores item properties, names, descriptions, and icon data for all in-game items.

#### File Structure

The file consists of a sequence of `ItemEntry` structures, each representing a single item. The entire file is encrypted using a byte-wise rotate-right-by-5-bits (ROR5) operation. See [`FFXIDat/ItemData.cpp`](../FFXIDat/ItemData.cpp), functions `decryptRor5` and `encryptRol5`.

```
[ItemEntry 1][ItemEntry 2]...[ItemEntry N]
```

#### Encryption

All bytes in the file are encrypted using ROR5. On reading, each byte is rotated right by 5 bits; on writing, the inverse (ROL5) is applied. This is handled automatically by the code unless `encryptionSuppression` is enabled. See [`FFXIDat/ItemData.cpp`](../FFXIDat/ItemData.cpp).

#### Entry Format

Each entry is defined by the following structure (see [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h)):

```cpp
struct ItemEntry {
    ItemHeader header;           // Flags and basic properties (bitfields)
    ItemSpecData spec;           // Type-specific data (626 bytes, union)
    uint32_t image_length;       // Actual icon data size
    char image_data[2427];       // Icon bitmap (DXT-compressed, see Image.h)
    uint8_t end_marker;          // Always 0xFF
};
```

##### ItemHeader

The `ItemHeader` contains the item ID and a series of bitfields for flags (e.g., rare, ex, inscribable), stack size, item type, resource ID, and valid targets. See the `ItemHeader` struct in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h).

##### ItemSpecData

This is a union of several possible structures, selected according to the item type (e.g., weapon, armour, usable, puppet, slip, currency). Each spec contains a `Record` structure (see [`FFXIDat/Record.h`](../FFXIDat/Record.h)) that holds the text fields for the item. The correct spec is chosen based on the context or file type. See the `ItemSpecData` union and related structs in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h).

##### Text Fields (Name, Description, etc.)

Text fields are stored as a `Record` structure within the appropriate spec. The format of the `Record` varies by language:

- **Japanese files**: Two cells, both strings: `[Name, Description]`
- **English files**: Five or more cells: `[Name, LogFlag (int), Singular, Plural, Description]`

The code provides accessors for these fields (see `ItemDatum` class in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h)), which automatically select the correct cell based on the file format.

##### Job, Race, and Equipment Slot Applicability

These are stored as bitfields within the spec structures. See `ItemJobApplicability`, `ItemRaceApplicability`, and `ItemEquipSlot` in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h).

##### Image Data

The icon for each item is stored as a DXT-compressed bitmap in the `image_data` array. The actual length is given by `image_length`. The code validates that the length does not exceed the array size. See `Image` handling in [`FFXIDat/Image.h`](../FFXIDat/Image.h) and usage in [`FFXIDat/ItemData.cpp`](../FFXIDat/ItemData.cpp).

##### End Marker

Each entry must end with a byte of value `0xFF`. The code checks this for integrity.

#### Special Case: Currency Files

Currency files (see `ItemSpecType::CURRENCY` in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h)) have the following unique properties:

- The file size is always exactly 0xC000 (49152) bytes.
- There is exactly one `ItemEntry` in the file; the remainder is zero-padded.
- The code enforces these constraints on both read and write (see comments in [`FFXIDat/ItemData.cpp`](../FFXIDat/ItemData.cpp)).

#### References

- [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h): Structure definitions and accessors
- [`FFXIDat/ItemData.cpp`](../FFXIDat/ItemData.cpp): File reading/writing, encryption, and special cases
- [`FFXIDat/Record.h`](../FFXIDat/Record.h): Record and Row structures for text fields
- [`FFXIDat/Image.h`](../FFXIDat/Image.h): Image handling

---

### Status Data

**Location**: ROM/0/12 ROM/119/57 

**Purpose**: Status effect descriptions and icons

#### Entry Format

```cpp
struct StatusEntry {
    uint16_t id;                 // Status ID (ROR7)
    uint16_t flg;                // Flags
    StatusSpecData spec;         // Description record (ROR7)
    uint32_t image_length;       // Icon size (raw)
    char image_data[5499];       // Icon data (raw, no end marker)
    uint8_t end_marker;          // 0xFF
};
```

**Notes**:
- ID and spec data are stored using ROL7 (rotate left by 7 bits) encryption and decrypted using ROR7 (rotate right by 7 bits)
- Image data is stored unencrypted
- Fixed image size of 5499 bytes

---

### Records of Eminence

**Location**:
- Quest data: `ROM/307/15`
- Category data: `ROM/307/23`

**Purpose**: Stores objectives, rewards, and category information for the Records of Eminence system.

**Encryption**: The entire file is stored using ROL5 (rotate left by 5 bits) encryption. On reading, the file is decrypted using ROR5. See [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h).

#### File Structure

There are two main file types:

1. **Quest File (`ROM/307/15`)**: Contains individual quest/objective entries.
2. **Category File (`ROM/307/23`)**: Contains category entries that organise quests.

---

#### Quest Entry Format

Each quest entry is defined as follows (see `RoeQuestEntry` in [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h)):

```cpp
struct RoeQuestEntry {
    uint32_t id;                // Unique quest ID
    uint32_t release_date;      // Date in YYYYMMDD format
    uint32_t repeatable;        // 0 = not repeatable, 1 = repeatable
    uint32_t target_count;      // Number of targets required
    uint32_t emi_reward;        // Eminence points reward
    uint32_t exp_reward;        // Experience points reward
    uint32_t cap_reward;        // Capacity points reward
    uint32_t uni_reward;        // Unity points reward
    union {
        char raw[3039];
        Record info_rec;        // Text fields (see below)
    } info;
    char terminator;            // Must be 0xFF
};
```

**Text Fields**:
- Stored in the `info_rec` field as a `Record` structure.
- **Japanese files**: 3 cells (cell 0: quest name, cell 1: description, cell 2: unused)
- **English files**: 5 cells (cell 0 & 1: quest name, cell 2: unused, cell 3: description, cell 4: unused)
- Accessors for these fields are provided in the code (see `RoeQuestDatum` in [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h)).

**Rewards**:
- The various reward fields specify the points or experience granted upon completion.

---

#### Category Entry Format

Each category entry is defined as follows (see `RoeCategoryEntry` in [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h)):

```cpp
struct RoeCategoryEntry {
    uint32_t id;                    // Unique category ID
    uint32_t count_of_children;     // Number of child entries
    struct {
        uint32_t child_id;          // ID of child (quest or category)
        uint32_t quest_flag;        // 0 = category, non-zero = quest
        uint32_t ukn[3];            // Unknown, usually zero
    } children[28];
    union {
        char raw[2503];
        Record info_rec;            // Text fields (see below)
    } info;
    char terminator;                // Must be 0xFF
};
```

**Text Fields**:
- Stored in the `info_rec` field as a `Record` structure.
- Cell 0 contains the category name.

**Children**:
- Each category can reference up to 28 child entries, which may be other categories or quests.

---

#### Special Notes

- All entries are packed sequentially in the file.
- The terminator byte (0xFF) is used for integrity checking.
- The code preserves all unknown fields for round-trip fidelity.

#### References
- [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h): Structure definitions and accessors
- [`FFXIDat/RecordsOfEminence.cpp`](../FFXIDat/RecordsOfEminence.cpp): File reading/writing and handling
- [`FFXIDat/Record.h`](../FFXIDat/Record.h): Record and Row structures for text fields

---

### Monster Bridge

**Location**: ROM/27/38

**Purpose**: Monster display names and internal identifiers

#### Entry Format

```cpp
struct MonBridgeEntry {
    uint16_t id;                // Monster ID
    char internalName[32];      // ASCII identifier (DO NOT TRANSLATE)
    Record displayName;         // Localized display name
};
```


**Important**: The `internalName` field is used by game logic to identify monsters and must remain in ASCII. Only the `displayName` should be translated.

**Encryption**: The entire file is stored using ROL5 (rotate left by 5 bits) encryption. On reading, the file is decrypted using ROR5. See [`FFXIDat/MonBridge.cpp`](../FFXIDat/MonBridge.cpp).

---

## Menu and UI Formats

### Block Files

**Magic Header**: `menu` (4 bytes ASCII)

**Purpose**: Menu layouts, UI textures, lobby graphics

#### File Structure

```
[File Header][Block 1][Block 2]...[Block N][End Block]
```

#### File Header

```cpp
struct BlockFileHeader {
    char type[4];        // "menu"
    uint8_t flg1;
    uint8_t flg2;
    uint16_t ukn;
    uint32_t ukn1;
    uint32_t ukn2;
    uint32_t ukn3;
    uint32_t ukn4;
    uint32_t ukn5;
    uint32_t ukn6;
};
```

#### Block Header

```cpp
struct BlockHeader {
    char name[4];              // Block identifier
    uint32_t type : 7;         // Block type
    uint32_t size : 25;        // Block size in 16-byte units
    uint32_t padding[2];
};
```

**Block Types**:
- `0x20`: Image Block (texture data)
- `0x30`: Menu Form Block (layout)
- `0x31`: Image Set Block (texture references)
- `0x00`: End marker

---

#### Image Block (0x20)

Contains DXT-compressed texture data.

##### Image Header

```cpp
struct ImageHeader {
    uint8_t type;           // 0x91=Bitmap, 0xA1=DXT
    char group[8];          // Resource group
    char name[8];           // Resource name
    uint32_t version;       // Usually 0x28
    uint16_t width;
    uint16_t height;
    uint8_t mipmapCount;
    uint8_t bitCount;
    uint32_t ukn[6];
};
```

##### DXT Header (if type == 0xA1)

```cpp
struct DXTHeader {
    char fourCC[4];         // "DXT1", "DXT3", or "DXT5"
    uint32_t textureSize;   // Compressed data size
    uint32_t pitch;         // Row alignment
};
```

**Supported Compressions**:
- **DXT1**: 4x4 blocks, 8 bytes per block, 1-bit alpha
- **DXT3**: 4x4 blocks, 16 bytes per block, 4-bit alpha
- **DXT5**: 4x4 blocks, 16 bytes per block, interpolated alpha

---

#### Image Set Block (0x31)

References multiple textures and defines how to clip/tile them.

##### Structure

```cpp
struct ImageSetBlock {
    char group[8];                  // Image set group
    char name[8];                   // Image set name
    uint8_t refCount;               // Number of referenced textures
    char refTextures[refCount][16]; // Texture names
    
    // Followed by clip/tile data
    uint8_t type;                   // 0x74
    uint16_t groupCount;            // Number of tile groups
    
    // For each group:
    //   uint8_t imageCount
    //   ImageRef[imageCount]
};
```

##### Image Reference

```cpp
struct ImageRef {
    Vec2<int16_t> tlPoint;    // Top-left vertex
    Vec2<int16_t> trPoint;    // Top-right vertex
    Vec2<int16_t> blPoint;    // Bottom-left vertex
    Vec2<int16_t> brPoint;    // Bottom-right vertex
    uint16_t w;               // Source width
    uint16_t h;               // Source height
    uint16_t x;               // Source X offset
    uint16_t y;               // Source Y offset
    uint8_t type;             // Flip flags (0=normal, 1=H, 2=V, 3=both)
    RGBA tlColour;            // Vertex color modulation
    RGBA trColour;
    RGBA blColour;
    RGBA brColour;
    uint8_t ukn[4];           // Unknown (ukn[1]==0x2 may be silhouette flag)
    char group[8];            // Source texture group
    char name[8];             // Source texture name
};
```

**Notes**:
- Coordinates define a quadrilateral for perspective-correct texture mapping
- Color values are multiplied: 127 = 100%, 255 = 200%
- Alpha is clamped: values > 127 are treated as 127

---

#### Menu Form Block (0x30)

**Status**: Partially documented

Contains layout information for menu elements. Structure varies by menu type.

```cpp
struct MenuLayoutBlock {
    char group[8];
    char name[8];
    uint8_t dstCount;
    uint8_t srcCount;
    uint8_t padding[14];
    // Variable-length facet data follows
};
```

---

## Encoding Notes

### Shift-JIS Extensions

Square Enix extended Shift-JIS to include French and German characters:
- Accented characters mapped to unused Shift-JIS ranges
- Full mapping table needed for proper localization

### String Control Codes

Many string formats support inline control codes for:
- Text color changes
- Delays and pauses
- Item/spell name insertion
- Player name insertion
- Auto-advance

Format and behavior vary by string type. See game disassembly for details.

---

## Encryption Methods


### ROL (Rotate Left) and ROR (Rotate Right)

Some files are stored using ROL (rotate left) encryption and must be decrypted using ROR (rotate right) when reading.

**ROL5/ROR5** (used for ItemData, MonBridge, RecordsOfEminence):
```cpp
// Encryption (on write):
uint8_t rol(uint8_t value, int bits) {
    return (value << bits) | (value >> (8 - bits));
}
// Decryption (on read):
uint8_t ror(uint8_t value, int bits) {
    return (value >> bits) | (value << (8 - bits));
}
```

**ROL7/ROR7** (used for StatusData ID and spec):
Same as above, but with 7 bits.

### XOR Obfuscation

Some files (e.g., DMsg, EventString) use simple XOR obfuscation:
```cpp
// DMsg: XOR with 0xFF if obs field is set
for (int i = 0; i < size; i++) {
    data[i] ^= 0xFF;
}
// EventString: XOR with 0x80 if flag is set
for (int i = 0; i < size; i++) {
    data[i] ^= 0x80;
}
```

Refer to the relevant code files for implementation details.

---

## Tools for Viewing

- **FFXIDatProcessor**: Extract to CSV for all formats
- **FFXIMenu**: GUI editor for block files
- **TexHammar**: External tool for viewing DDS textures
- **AltanaViewer**: External viewer for rendered image sets

---

## Contributing

Format documentation is incomplete in many areas. Contributions welcome:
- Unknown fields in headers
- Control code meanings
- Menu layout structure
- Additional file types

Please submit findings with hex dumps and test cases.
