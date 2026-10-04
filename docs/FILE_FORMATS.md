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

The file is a sequence of fixed size slots, each holding one item record. The whole file is encrypted with a byte-wise rotate-right-by-5-bits (ROR5) operation that is applied to every slot. See `slotfile::Ror5Cipher` in [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h).

```
[ItemEntry 1][ItemEntry 2]...[ItemEntry N]
```

#### Encryption

All bytes in the file are encrypted using ROR5. On reading, each byte is rotated right by 5 bits; on writing, the inverse (ROL5) is applied. This is handled automatically unless `encryptionSuppression` is enabled. See `slotfile::Ror5Cipher` in [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h).

#### Entry Format

The v30 entry (`itmfmt::v30::Entry` in [`FFXIDat/ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h)) is:

```cpp
struct Entry {
    Header header;               // 16 bytes: id, two flag bytes, extended_flags,
                                 //           stack_size, item_type, resource_id, valid_targets
    SpecData spec;               // 624 bytes, union of the eight per type specs
    uint32_t image_length;       // offset 640: actual icon data size
    char image_data[4475];       // offset 644: icon bitmap (DXT-compressed, see Image.h)
    uint8_t end_marker;          // offset 5119: always 0xFF
};
```

The older layouts (`v10`, `v20`) use a 14 byte header (the same fields without `extended_flags`), a
626 byte spec union and a 2427 byte icon area, which is why their slot is 0xC00 bytes. Their spec
area is kept as opaque bytes: only the header and the position of the text record are modelled, so
those records are rewritten byte for byte without guessing their field meanings.

##### ItemHeader

The header carries the item ID, two bytes of flag bitfields (e.g., rare, ex, inscribable), and the 16 bit fields `extended_flags` (v30 only, at offset 6), `stack_size`, `item_type`, `resource_id` and `valid_targets` (offsets 8/10/12/14 in v30, 6/8/10/12 in v10 and v20). See `itmfmt::v30::Header` in [`FFXIDat/ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h); [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h) re-exports it as `ItemHeader`.

##### ItemSpecData

This is a union of several possible structures, selected according to the item type (e.g., weapon, armour, usable, puppet, slip, currency). Each spec contains a `Record` structure (see [`FFXIDat/Record.h`](../FFXIDat/Record.h)) that holds the text fields for the item. The correct spec is chosen based on the context or file type. See the `ItemSpecData` union and related structs in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h).

##### Text Fields (Name, Description, etc.)

Text fields are stored as a `Record` structure at the text offset of the record version (see the table below). The number and meaning of the cells depends on the language of the table:

- **Japanese files**: Two cells, both strings: `[Name, Description]`
- **English files**: Five or more cells: `[Name, LogFlag (int), Singular, Plural, Description]`
- **French files**: Six cells (name, log flag, singular, plural, and the description in cell 5)
- **German files**: Nine cells (description in cell 8)

The datum (`itmfmt::Datum`) provides accessors for these fields (`name`, `name_sg`, `name_pl`, `description`, `logFlag`), which pick the cell from the cell count they observe. See [`FFXIDat/ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h).

##### Job, Race, and Equipment Slot Applicability

These are stored as bitfields within the spec structures. See `ItemJobApplicability`, `ItemRaceApplicability`, and `ItemEquipSlot` in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h).

##### Image Data

The icon for each item is stored as a DXT-compressed bitmap in the `image_data` array at offset 644. The actual length is given by the 32 bit field at offset 640. The code validates that the length does not exceed the array size. See `Image` handling in [`FFXIDat/Image.h`](../FFXIDat/Image.h) and usage in [`FFXIDat/ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h).

##### End Marker

Each slot must end with a byte of value `0xFF` (the last byte of the slot). The container checks this marker for every slot before it parses any record.

#### Special Case: Currency Files

Currency files (see `ItemSpecType::CURRENCY` in [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h)) have the following unique properties:

- The file size is exactly `Format::currencySlots` slots (16): 0x14000 bytes on the v30 layout and 0xC000 bytes on the v10 and v20 layouts.
- There is exactly one entry in the file; the remainder is zero-padded.
- The container enforces these constraints on both read and write (see `slotfile::SlotFile` in [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h)).

#### Record Versions

Three record layouts of this family have been observed:

| Version | Layout header | Slot size | Header | Text record offset (`inb` / `iub` / `iwb` / `iab` / `ipb` / `isb` / `icb` / `iib`) | Icon capacity | Used by |
|---|---|---|---|---|---|---|
| `v30` (newest known) | [`ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h) | 0x1400 | 16 bytes | 28 / 28 / 60 / 48 / 28 / 84 / 20 / 44 | 4475 | ja/en tables after the 2026-09 update |
| `v20` | [`ItemFormatV20.h`](../FFXIDat/ItemFormatV20.h) | 0xC00 | 14 bytes | 24 / 28 / 56 / 44 / 24 / 84 / 16 / 40 | 2427 | ja/en tables before the 2026-09 update |
| `v10` (oldest known) | [`ItemFormatV10.h`](../FFXIDat/ItemFormatV10.h) | 0xC00 | 14 bytes | 24 / 24 / 48 / 40 / 24 / 84 / 16 / 40 | 2427 | the de/fr tables left in the live client |

#### References

- [`FFXIDat/ItemData.h`](../FFXIDat/ItemData.h): facade, compatibility names and routing
- [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h): the slot container shared by items, RoE and MonBridge
- [`FFXIDat/ItemFormats.h`](../FFXIDat/ItemFormats.h): version aliases and the routing table
- [`FFXIDat/ItemFormatV30.h`](../FFXIDat/ItemFormatV30.h), [`FFXIDat/ItemFormatV10.h`](../FFXIDat/ItemFormatV10.h): the layout of each version
- [`FFXIDat/ItemFormats.cpp`](../FFXIDat/ItemFormats.cpp): the CSV view of this family
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
- The slot is 0x1800 (6144) bytes and every slot ends with a `0xFF` marker, like the families above; the shared container (`slotfile::SlotFile`) does not implement this cipher, so StatusData keeps its own reader (see [`FFXIDat/StatusData.cpp`](../FFXIDat/StatusData.cpp)).
- ID and spec data are stored using ROL7 (rotate left by 7 bits) encryption and decrypted using ROR7 (rotate right by 7 bits)
- Image data is stored unencrypted
- Fixed image size of 5499 bytes

---

### Records of Eminence

**Location**:
- Quest data: `ROM/307/15`
- Category data: `ROM/307/23`

**Purpose**: Stores objectives, rewards, and category information for the Records of Eminence system.

**Encryption**: The file is stored with the same slot rotation as the other families: ROR5 on reading, ROL5 on writing. See `slotfile::Ror5Cipher` in [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h).

#### File Structure

There are two main file types:

1. **Quest File (`ROM/307/15`, type `erq`)**: Contains individual quest/objective entries.
2. **Category File (`ROM/307/23`, type `erc`)**: Contains category entries that organise quests.

Three record layouts of this family have been observed:

| Version | Layout header | Slot size | Text record offset (quest / category) | Used by |
|---|---|---|---|---|
| `v30` (newest known) | [`RoeFormatV30.h`](../FFXIDat/RoeFormatV30.h) | 0x1400 | 32 / 568 | ja/en tables after the 2026-09 update |
| `v20` | [`RoeFormatV20.h`](../FFXIDat/RoeFormatV20.h) | 0xC00 | 32 / 568 | ja/en tables before the 2026-09 update |
| `v10` (oldest known) | [`RoeFormatV10.h`](../FFXIDat/RoeFormatV10.h) | 0xC00 | 28 / 568 | de/fr tables of the live client, selected by the `_o` type suffix |

The category record is the same in `v10` and `v20`, which is why `RoeFormatV20.h` reuses the `v10`
type for it. Only the quest record gained `uni_reward` in `v20` (see below).

Actually there is no v20 for category or the `v30` in the code is actuaally the v2 of this file,
the only reason why it got v30 instead of v20 is try to keep the version same with the quest file, 
reduce the cost to understand there is only two version of the category file whilst the quest file has three version.

---

#### Quest Entry Format

Each quest entry is defined as follows (see `roefmt::v30::QuestEntry` in [`FFXIDat/RoeFormatV30.h`](../FFXIDat/RoeFormatV30.h); `RoeQuestEntry` in [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h) is the same type):

```cpp
struct QuestEntry {
    uint32_t id;                // Unique quest ID
    uint32_t release_date;      // Date in YYYYMMDD format
    uint32_t repeatable;        // 0 = not repeatable, 1 = repeatable
    uint32_t target_count;      // Number of targets required
    uint32_t emi_reward;        // Eminence points reward
    uint32_t exp_reward;        // Experience points reward
    uint32_t cap_reward;        // Capacity points reward
    uint32_t uni_reward;        // Unity points reward (v20 and v30 only)
    union {
        char raw[5087];         // 5087 here, 3039 in v20, 3043 in v10
        Record info_rec;        // Text fields (see below)
    } info;
    char terminator;            // Must be 0xFF
};
```

The `v10` record has no `uni_reward` field: its reward block ends at `cap_reward`, which is why its
text record sits 4 bytes earlier and its text capacity is 4 bytes larger than `v20`'s.

**Text Fields**:
- Stored in the `info_rec` field as a `Record` structure.
- **Japanese files**: 3 cells (cell 0: quest name, cell 1: description, cell 2: unused)
- **English files**: 5 cells (cell 0 & 1: quest name, cell 2: unused, cell 3: description, cell 4: unused)
- Accessors for these fields are provided in the code (see `roefmt::QuestDatum` in [`FFXIDat/RoeFormatV30.h`](../FFXIDat/RoeFormatV30.h)).

**Rewards**:
- The various reward fields specify the points or experience granted upon completion.

---

#### Category Entry Format

Each category entry is defined as follows (see `roefmt::v30::CategoryEntry` in [`FFXIDat/RoeFormatV30.h`](../FFXIDat/RoeFormatV30.h); `RoeCategoryEntry` in [`FFXIDat/RecordsOfEminence.h`](../FFXIDat/RecordsOfEminence.h) is the same type, and `v20` reuses the `v10` one):

```cpp
struct CategoryEntry {
    uint32_t id;                    // Unique category ID
    uint32_t count_of_children;     // Number of child entries
    struct {
        uint32_t child_id;          // ID of child (quest or category)
        uint32_t quest_flag;        // 0 = category, non-zero = quest
        uint32_t ukn[3];            // Unknown, usually zero
    } children[28];
    union {
        char raw[4551];             // 4551 in v30, 2503 in v10 and v20
        Record info_rec;            // Text fields (see below), cell 0 is the category name
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

**Location**: `ROM/288/66` (ja/en, type `mbd`) and `ROM/288/68` (de/fr, type `mbd_o`)

**Purpose**: Monster display names and internal identifiers

**Encryption**: The file is stored with the same slot rotation as the other families: ROR5 on reading, ROL5 on writing. See `slotfile::Ror5Cipher` in [`FFXIDat/SlotFile.h`](../FFXIDat/SlotFile.h).

#### Record Versions

| Version | Layout header | Slot size | Name field | Text record | Text capacity | Icon length / data |
|---|---|---|---|---|---|---|
| `v30` (newest known) | [`MonBridgeFormatV30.h`](../FFXIDat/MonBridgeFormatV30.h) | 0x1400 | offset 8 | offset 116 | 524 | 640 / 644 |
| `v10`, `v20` (oldest known) | [`MonBridgeFormatV10.h`](../FFXIDat/MonBridgeFormatV10.h) | 0xC00 | offset 6 | offset 112 | 528 | 640 / 644 |

`v20` is an alias of `v10` in this family: the ja/en tables before the 2026-10 update used the same
record as the de/fr tables of that era, so both version labels select that layout. The `_o` type
suffix selects it as well.

#### Entry Format

The v30 entry (`mbfmt::v30::Entry`, 0x1400 bytes):

```cpp
struct Entry {
    uint32_t id;
    uint16_t idx;               // at offset 4 in every version
    uint16_t ukn0;              // always zero in observed data
    char name[32];              // ASCII identifier (DO NOT TRANSLATE)
    int16_t para1[5];
    uint16_t ukn1;              // always zero in observed data
    int8_t para2[64];
    union {
        char raw[524];          // 524 here, 528 in v10 and v20
        Record info_rec;        // Localized display name (cell 0)
    } rec;
    uint32_t icon_size;         // offset 640: size of icon_data, 0 if no icon
    char icon_data[4475];       // offset 644 (2427 bytes in v10 and v20)
    char terminator;            // Must be 0xFF
};
```

Two 16 bit fields (`ukn0` at 6, `ukn1` at 50, both always zero in the observed data) were inserted
in the ja/en record, which moves `name` from offset 6 to 8, `para2` from 48 to 52 and the text record
from 112 to 116; the older record has neither field.

**Important**: The `name` field is used by game logic to identify monsters and must remain in ASCII. Only the display name (cell 0 of the text record) should be translated.

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
