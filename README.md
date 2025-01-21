# FFXIDat

A tool to extract and rebuild DAT files used in FFXI. Or at least I hope this tool can do so in the future.

## Features
- Ability to scan and export all known DAT files.
	- Event String Data
	- ```XISTRING``` files (system messages.)
	- ```d_msg``` files (system messages, menu texts.)
- Ability to rebuild xistring and d_msg files.

### Future Work
- Ability to extract/rebuild ```menu``` files. Item data is saved in these files, need analysis to extract them.
- Find out where the battle info stores
- Implement operation of font file (probably not)
- extract/rebuild menu images to modify the UI? (someone might already done this, find some info before start...)

### Known DAT Formats
Currently only pure text files can be processed.

- [x] XISTRING file (magic header: ```XISTRING```)
	- [x] Export
	- [ ] String Control Sequence Parse
	- [x] Import
- [x] DMsg file (magic header: ```d_msg```)
	- [x] Export
	- [x] Import
- [x] String Data for Events (per area, e.g. ROM/22/17.DAT)
	- [x] Export
	- [ ] String Control Sequence Analysis (In Progress)
	- [x] Import

#### DMsg file
```DMsg.cpp``` ```DMsg.h```.
```
[Header][Index(Optional)][Row][Row][Row]...
```
```
[Num of Cells][Cell1 Metadata][Cell2 Metadata]...[Cell1 Data][Cell2 Data]...
```

#### XISTRING
```cpp
struct XiStringHeader {
	char magicHeader[8];
	int32_t version; // always 0x20000
	int32_t zero[5]; // always 0
	int32_t fileSize;
	int32_t entriesCount;
	int32_t indicesSize;
	int32_t dataSize;
	int32_t reserved; // always 0
	int32_t id; // seems an id ? Xistring files have same perpose have the same id
};

struct XiStringIndex {
	int32_t offset;
	uint16_t size;
	uint16_t flag1;
	uint16_t flag2;
	uint16_t flag3;
};
```
File structure is:
```
Header
Index
Strings
```
Index comes with the header. The ```offset``` is relative to the beginning of ```Strings``` block.

#### Events Strings
```
int24 fileSize
int8  flag
int32 offsets[N]
```
* ```flag``` is always 0x10, the purpose of which is unclear
* ```offsets``` indicates the byte offset of the corresponding string relative to the starting point of ```offsets```.
* All bytes are xor-ed with 0x80, except for ```fileSize``` and ```flag```.

## Note
Square extended the SJIS to represent characters in FR and DE. Further research required.
