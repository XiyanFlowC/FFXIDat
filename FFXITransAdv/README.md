# FFXITransAdv

FFXI event dialogue extraction / write-back tool. Uses FFXIDatAdv bytecode analysis engine and FFXITrans translation pipeline. Supports event-aware processing with dual-language output and FFXITrans-compatible interactive mode.

## CLI Usage

```
FFXITransAdv.exe extract [zone] [--lang ja|en]  Extract event dialogue to TXT
FFXITransAdv.exe apply   [zone]                  Write translated TXT back to DAT
FFXITransAdv.exe prepare [ja|en]                 Prepare source text for translation
FFXITransAdv.exe list                             List all event zones
FFXITransAdv.exe                                  Full processing (FFXITrans compatible)
```

## Interactive Mode

When run with no arguments, FFXITransAdv behaves identically to FFXITrans:
- Setup wizard (if config.ini is missing)
- InSitu yes/no prompt with double-confirmation
- Backup management
- Full file processing with summary

## Event-Aware Processing

When `event/` data is present, EventFileProcessor handles evsb files with paired evev:
- Parses evev/evac to identify actors and events
- Reads translated text from `text/tgt/event/` using ref.csv paths
- Applies per-event translation patches, falls back to TranslationDatabase

## Output Structure

```
texts/event/
  common/                  ← Cross-zone common actors (text dedup)
    {actorName}/
      {aidx}.txt
  {zoneName}/              ← Per-zone actors
    {actorName}_{actorId}/
      {aidx}.txt
```

## Dependencies

- Visual Studio 2022 (v143), C++20
- FFXI DAT installation (registry: `SOFTWARE\WOW6432Node\PlayOnline\InstallFolder`)
- `cp932.csv` — Shift-JIS ↔ UTF-8 codepage table (in exe directory)
- `defs.csv` — Zone configuration (in exe directory)

## Build

```bash
msbuild FFXITransAdv.vcxproj /p:Configuration=Release /p:Platform=x64
```

Copy `data/defs.csv` and `cp932.csv` to `x64/Release/` before running.

## License

See repository LICENSE.
