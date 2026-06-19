# FFXIDatAdv

FFXI event dialogue extraction tool. Extracts dialogue data from FFXI event binary files (evev/evac/evsb), organized by Actor (NPC/entity), outputting structured JSON and TXT.

## Data Flow

```
defs.csv → ZoneConfig (zone definitions)
    ↓
GamePathResolver → resolve ROM/X/Y paths to filesystem
    ↓
ZoneEventImage (evev) → ActorBlock[] (actor_id, constants, events) — FFXIDat, clean-room
ZoneActor (evac)       → name map (actor_id → name)          — FFXIDat, clean-room
EventStringBase (evsb) → string[] (localized text)
    ↓
EventLinker / BytecodeAnalyzer → extract DialogueLine from bytecode
    ↓
EventAnalyzer → dedup dialogues per event, build textLines
    ↓
DataDumper::Flush → JSON + TXT output
```

## CLI Usage

```
FFXIDatAdv.exe [--help]                    Show help
FFXIDatAdv.exe [zone]                      Process a single zone
FFXIDatAdv.exe                             Process all zones
FFXIDatAdv.exe --dump-db --lang jp|na      Export event database (TXT + JSON)
FFXIDatAdv.exe --dump-event-json           Export event database (dual-language JSON)
  --out <dir>                              Output directory
  --list-zones                             List all available zones
  --dump-opcodes <zone>                    Dump bytecode opcodes for a zone
  --split-text                             Split text output by language
  --pretty                                 Pretty-print JSON output
  --ffxi-path <path>                       Set FFXI installation path
```

## Output Structure

### `--dump-db` (TXT + JSON)
```
<outDir>/
  event/
    text/
      {zoneName or "common"}/
        {actorDir}/
          {aidx}.txt              ← deduped dialogue lines
    {zoneName or "common"}/
      {actorDir}.json             ← Actor metadata + dialogue index
    ref.csv
  evsb_msgs.txt                   ← evsb strings not referenced by any dialogue
  ev/, gev/                       ← orphan evsb files (zones without evev)
  etc/, sys/, itm/, ...           ← system data (CSV/TXT)
```

### `--dump-event-json` (JSON, dual-language)
```
<outDir>/event/
  txt/
    ja/                           ← Japanese text JSON arrays
      {zoneName or "common"}/{actorDir}/{aidx}.json
    en/                           ← English text JSON arrays (same structure)
  {zoneName or "common"}/         ← Actor JSONs
    {actorDir}.json
  ref.csv
```

### Actor JSON format
```json
{
  "actor": "Zone Events",
  "actor_number": 2147483632,        // or "actor_numbers": [...] for common actors
  "speakers": ["???", "Zone Events"],
  "events": [{
    "event_id": 47,
    "array_index": 13,
    "txt": "ev/Inner Horutoto Ruins/Zone Events/13.json",
    "evsb_refs": [7],
    "dialogues": [{"speaker": 1, "line": 0}]
  }]
}
```

## Common Actor Deduplication

Actors with identical `actor_name` and `bytecode_hash` appearing in 2+ zones are promoted to `common/`. Text content is additionally deduplicated per-event via content hash, sharing JSON files across zones.

## Clean-Room Parsers (FFXIDat)

`ZoneEventImage` and `ZoneActor` are independent, clean-room implementations in FFXIDat. They parse evev (actor IDs, constants/imed, events) and evac (actor name catalog) without any opcode analysis. No AGPL-licensed code was used.

## Build

- Visual Studio 2022 (v143), C++20
- Dependencies: FFXIDat.lib, xybase.lib (sibling projects in workspace)

## References

- [POLUtils](https://github.com/Windower/POLUtils) — POL/FFXI Data utilities
- [XiEvents](https://github.com/atom0s/XiEvents) — FFXI event system reverse-engineering docs
- [FFXI-EventsDump](https://github.com/sruon/FFXI-EventsDump/) — Python event extraction tool

## License

See repository LICENSE.
