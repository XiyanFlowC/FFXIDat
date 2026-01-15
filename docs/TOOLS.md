# Tool Usage Guide

This document describes the command-line tools included in the repository and how to use them for common tasks (extract, edit, rebuild, and translation workflows).

Contents
- `FFXIDatProcessor` ！ CLI for conversions and SQLite operations
- `FFXITrans` ！ translation injection tool for in-place or output workflows
- `FFXIMenu` ！ GUI tool (overview)
- Typical workflows
- Configuration and notes

FFXIDatProcessor
-----------------
FFXIDatProcessor is the primary command-line tool for converting DAT files to editable text (CSV), for importing edited CSV back into DAT, and for interacting with the translation database.

Build and run
- Build the `FFXIDatProcessor` project in `FFXIDat.sln`.
- Run from the build output folder or add the executable to your PATH.

Common commands
- Extract/convert DAT -> CSV
  - `FFXIDatProcessor.exe --dmsg-to-csv path/to/file.DAT`
  - `FFXIDatProcessor.exe --xis-to-csv path/to/file.DAT`
  - For many supported formats the tool detects the file type automatically when you pass a `.DAT` file directly.

- Convert CSV -> DAT
  - `FFXIDatProcessor.exe --csv-to-dmsg edited_file.csv`
  - `FFXIDatProcessor.exe --csv-to-xis edited_file.csv`
  - `FFXIDatProcessor.exe --csv-to-fp edited_file.csv` (FixedPhrase)

- Database / SQLite operations
  - Initialize DB from CSV definitions (creates or replaces `text.db`):
    - `FFXIDatProcessor.exe --sql-init defs.csv`
  - Update file definitions: `--sql-file-update defs.csv`
  - Purge unreferenced text entries: `--sql-purge`
  - Export translation data: `--sql-trans-dump`
  - Export entries without translation: `--sql-trans-dump-empty`
  - Import translations into DB: `--sql-trans-import`
  - Translate dat files using DB and write outputs: `--sql-dat-trans` (flag `-T`)
  - Read dats into DB according to conditions: `--sql-dat-read` (flag `-q`) with optional condition flags:
    - `--sql-cond-type <type>` (e.g. `evsb`)
    - `--sql-cond-lang <lang>` (e.g. `jp` or `en`)
    - `--sql-cond-path <path>` (path pattern)

- Misc flags
  - `-x` / `--do-xor` : enable XOR protection for DMsg when writing
  - `-b` / `--block`  : write DMsg in block mode
  - `--scan-extract`  : scan and export known DAT files from a game install
  - `-I <path>` / `--install-path <path>` : specify game installation path (useful when extracting from game files)
  - `--help` or `-?` : show help

Notes
- When FFXIDatProcessor is passed a non-option argument (not starting with `-`), it will attempt to auto-detect file type by header and convert accordingly (CSV output). This is convenient for batch conversion.

FFXITrans
---------
FFXITrans is a translation insertion tool designed to apply translations to game DATs from prepared translation files.

Basic usage
- Interactive mode: `FFXITrans.exe` (no args)
- In-place modification mode: `FFXITrans.exe insitu`

Behavior and configuration
- `FFXITrans` attempts to read a `config.ini` file in the program directory. Recognized keys:
  - `game_path` ！ game installation directory (overrides registry detection)
  - `in_situ` ！ if `1`/`true` then write in-place and suppress prompts
  - `english_mode` ！ if `1`/`true` process English (EU/US) data
  - `output_path` ！ where to write output when not in-situ

- It uses `defs.csv` (in program folder) to map file types and paths for batch processing. The file lists which DATs to process and with which language.

- `FFXITrans` creates `text_mismatch.txt` in the program folder to record text entries that did not have a matching translation.

Configuration files and paths
- `config.ini` ！ optional, described above
- `defs.csv` ！ file definitions mapping used by processor tools
- `cp932.csv`, `chs2sjis.csv` ！ codepage and Chinese->SJIS mapping files used by tools; placed in the program folder (referenced by code at runtime)

Workflow
- Run `FFXITrans.exe` with configured `defs.csv` once
- Translate generated `text_mismatch.txt`
- Rename the `text_mismatch.txt` to `text.txt` as original text file and the translation to `text_translated.txt`
- Run `FFXITrans.exe` again to inject translations

Backups
- `FFXITrans` will create backups under `backup` in the program directory when doing in-place edits. The tool may prompt to restore backups on next run.

FFXIMenu (GUI)
--------------
`FFXIMenu` is a Windows GUI application for inspecting and editing menu block files (textures, clips, layouts). It is MFC-based.

Capabilities
- Open block files that start with the `menu` header
- Inspect image blocks and export DDS
- Import DDS to replace textures
- Inspect image sets and tile/clip definitions

Notes
- The image decoding code supports `DXT1`, `DXT3` and `DXT5` formats. Some texture variants may be unimplemented.
- The GUI is intended for interactive editing; follow on-screen commands for exporting/importing textures and saving.

Where to look in the source
- `FFXIDatProcessor/FFXIDatProcessor.cpp` ！ command-line options and primary logic
- `FFXIDat/` ！ format implementations (`DMsg`, `XiString`, `EventString`, `ItemData`, etc.)
- `FFXITrans/FFXITrans.cpp` ！ translation injection logic
- `FFXIMenu/` ！ GUI editor implementation

