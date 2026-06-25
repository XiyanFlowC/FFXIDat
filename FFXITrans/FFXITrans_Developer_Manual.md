# FFXITrans Developer Manual

## 1. Introduction and Purpose

FFXITrans is a specialized translation pipeline tool for Final Fantasy XI (FFXI) game data files. It extracts, translates, and injects text from/into proprietary DAT formats (XiString, DMsg, EventStringBase, ItemData, etc.).

It is designed for **Chinese localization** (Simplified Chinese → Shift-JIS) but the architecture is general.

Key goals for developers:
- Pluggable per-type processors
- Layered translation lookup (global DB + per-file local scope + reference matching)
- Rule-based post-processing and validation (FinalTextProcessor)
- Safe backup + in-situ or output-to-folder modes
- Support for both "dumb" line-by-line and "smart" event-aware translation

Related projects in the repo:
- `FFXIDat/` — clean-room parsers (used by FFXITrans)
- `FFXIDatProcessor/` — database-oriented extraction (separate)
- `FFXITransAdv/` — advanced/event-heavy variant

---

## 2. High-Level Architecture

```
defs.csv (FileProcessDef list)
       │
       ▼
Application::ProcessTranslations()
       │
       ├── 1. EjrefToleranceProcessor (conditional, English + ejref_tolerance)
       │
       ├── 2. SpecialProcessor (hard-coded special cases: sys/job, etc.)
       │
       ├── 3. EventProcessor (evsb + paired evev/evac → event-aware patches)
       │
       └── 4. Factory processor by type (Xis, DMsg, Evsb fallback, Items, etc.)
                    │
                    ▼
              TranslationDatabase lookup
                    │
                    ▼
              FinalTextProcessor (rules + validation + babel)
                    │
                    ▼
              Write back to DAT (or output/)
```

**Core components**:
- `ProcessorFactory` — type → processor mapping (singleton)
- `FileProcessor` — abstract base for all type processors
- `TranslationDatabase` — global + local (per-comment) translation maps
- `FinalTextProcessor` — per-(comment,type) rule engine + validation
- `SpecialProcessor` / `EventProcessor` — early special-case handlers
- `Config` — central configuration (config.ini + command line)
- `ProcessorUtils` — shared helpers (cell indices, string collection, etc.)

---

## 3. Processor Execution Priority / Order

The order is **strict** and defined in `Application::ProcessTranslations()` (around lines 1013-1082).

### Exact Priority (per file)

| Step | Processor                    | When Active                                      | Returns true?          | Notes |
|------|------------------------------|--------------------------------------------------|------------------------|-------|
| 1    | `EjrefToleranceProcessor`    | English mode + `en_as_ja=true` + `ejref_tolerance=true` + specific comments | Yes (handled) | Special tolerance logic for mismatched EN/JA counts, shorter references, key items, etc. |
| 2    | `SpecialProcessor`           | Always (if step 1 didn't handle)                | Yes if matched comment | Currently only `sys/job` (job name hacks for Samurai/Monk etc.) |
| 3    | `EventProcessor`             | Always (if previous didn't handle)              | Yes if evsb (or with evev) | Strongest for `evsb`. If paired evev/evac exists, uses `text/src/event/` + `text/tgt/event/` per-actor patches. Falls back to normal DB lookup otherwise. |
| 4    | `ProcessorFactory::GetProcessor(type)` | If none of the above claimed it | N/A (side-effect) | The "normal" path. One shared instance per type group (e.g. all `i*` items share `ItemProcessor`). |

**Important notes on priority**:
- Once a processor returns `true` (or successfully processes), the file is considered **done** for that pass.
- `EventProcessor` is deliberately placed before the generic factory so that `evsb` can get event-structured treatment.
- `EvsbProcessor` (registered in factory as fallback for `evsb`) is only reached if `EventProcessor` did **not** handle the file (i.e., no evev or it explicitly returned false).
- `FinalTextProcessor` is **inside** almost every concrete processor (after DB lookup, before writing). It is **not** a top-level processor.

### Per-Processor Internal Flow (typical)

```cpp
// Inside XisProcessor / DMsgProcessor / etc.
auto& db = TranslationDatabase::Instance();
FinalTextProcessor final(fileDef.comment, fileDef.type);

// ... read data ...

for (each string s) {
    std::u8string trans = db.GetTranslation(s);           // or reference variant
    // local scope already loaded by some processors
    s = final.Process(trans, s, row, col);
}

// write
```

---

## 4. List of All Processors and Their Responsibilities

### Registered in `ProcessorFactory::RegisterDefaultProcessors()`

| Type(s)          | Class                    | Shared Instance? | Responsibility |
|------------------|--------------------------|------------------|----------------|
| `xis`            | `XisProcessor`           | No               | Menu / UI strings (XiString format) |
| `evsb`           | `EvsbProcessor` (fallback) + `EventProcessor` | No | Event strings. `EventProcessor` takes precedence when evev present. |
| `dmsg`           | `DMsgProcessor`          | No               | Dialog / system messages. Supports cell selection via defs.csv. |
| `iab,iwb,iub,inb,ipb,isb,icb,iib` | `ItemProcessor` | **Yes** (shared) | All item data tables (name + description). Cell control supported. |
| `sd`             | `StatusDataProcessor`    | No               | Status effect descriptions |
| `fp`             | `FixedPhraseProcessor`   | No               | 定型文 (fixed phrases) |
| `mbd`            | `MonBridgeProcessor`     | No               | Monstrosity / Monipulator names |
| `erq`            | `RoeProcessor` (shared)  | **Yes**          | Records of Eminence Quests |
| `erc`            | `RoeProcessor` (shared)  | **Yes**          | Records of Eminence Categories |

### Special / Early Processors (not in factory map)

| Name                        | Trigger                          | Key Logic |
|-----------------------------|----------------------------------|-----------|
| `EjrefToleranceProcessor`   | English + en_as_ja + ejref_tolerance + special comments (`gev/*`, certain dmsg, `sys/key_item`, etc.) | Uses JA reference with modulo / shorter reference tricks. Handles 2x record cases for English. |
| `SpecialProcessor`          | Hard-coded comments (`sys/job`) | Job name abbreviation / special casing (Samurai as "侍", Monk as "僧", etc.). |
| `EventProcessor`            | `evsb` files                     | If `evev` + `evac` exist for the zone → loads per-actor patches from `text/{src,tgt}/event/<zone>/`. Falls back to plain DB lookup. |

### Supporting / Post-Processing

- **FinalTextProcessor** — Applied inside nearly every processor. Loads `rules/common.csv` + `rules/<comment>.csv`. Supports REP / REPRE / SET / SETNXT commands + occurrence ranges + regex + validation (EVSB switch/gender, control sequences, etc.).
- **ProcessorUtils** — `CollectStrings`, `ParseCellIndices`, `IsEjref*` helpers, etc.

---

## 5. Translation Lookup Priority (inside a processor)

When a processor asks for a translation:

1. **Local Scope** (highest) — loaded via `db.LoadLocalScope(text/src/<comment>.txt, text/tgt/<comment>.txt)`. Per-comment, per-run.
2. **Reference matching** (when `en_as_ja` + English) — `GetTranslationFromReference(source, jaReference)`.
3. **Main global mapping** — `text.txt` + `text_translated.txt` + numbered supplements (`text1.txt` etc.).
4. **Directory override** — `text/src/...` + `text/tgt/...` (loaded earlier into the global map or via local scope).

Fallback: if no translation found, original text is kept (and recorded in mismatch log).

`EjrefToleranceProcessor` and `EventProcessor` have their own reference logic before falling back to the above.

---

## 6. Configuration and Data Files (Developer View)

### defs.csv (loaded by `Application::LoadFileDefinitions`)

Format:
```
path,type,lang,comment[,cellIndices]
```

- `comment` is the **primary key** used for:
  - TranslationDatabase local scope
  - FinalTextProcessor rule files
  - JP reference lookup (`jpDefsByComment`)
  - Excludes
  - Event zone name derivation

Cell indices (1-based, `|` separated) are passed to processors for selective column translation.

### config.ini

See the user README for keys. Important for developers:
- `ejref_tolerance`
- `en_as_ja`
- `excludes`
- `ctrl_seq_check`
- `sys_job_workaround`
- `babel` / `bilingual`

### rules/ directory

CSV files consumed by `FinalTextProcessor::LoadRules()`:
- `rules/common.csv` — always loaded first (global rules)
- `rules/<comment>.csv` — per comment (e.g. `rules/sys/job.csv`)

Command types and execution order are documented in `rules/example.csv`.

### text/ directory structure

```
text/
├── src/               # directory override originals (relative path = comment)
├── tgt/               # directory override translations
├── src_/              # output of "prepare" mode
├── <comment>.txt      # for LoadLocalScope
└── event/             # EventProcessor patches
    └── <zone>/
        └── *.txt or per-actor
```

---

## 7. How to Add a New Processor (Developer Guide)

1. Create `Processors/MyNewProcessor.h` and `.cpp`
   - Inherit from `FileProcessor`
   - Implement `Process(...)` and `GetSupportedType()`
   - Inside `Process`:
     - Use `TranslationDatabase::Instance().GetTranslation(...)`
     - Wrap result with `FinalTextProcessor`
     - Handle `TryGetJapaneseReference` if needed
     - Respect `fileDef.cellIndicesStr`

2. Register in `ProcessorFactory::RegisterDefaultProcessors()`:
   ```cpp
   RegisterProcessor(u8"mytype", std::make_shared<MyNewProcessor>());
   ```

3. Add the type to `defs.csv` examples and documentation.

4. If it needs special early handling (before EventProcessor), add logic to `SpecialProcessor` or create a new top-level early processor (rare).

5. If it needs event / actor awareness, model after `EventProcessor`.

6. Add tests via `prepare` mode + sample translation files.

**Recommended pattern** (copy from `XisProcessor` or `DMsgProcessor`):
```cpp
FinalTextProcessor final(fileDef.comment, fileDef.type);
... for each string ...
s = final.Process( db.GetTranslation(s) , original, row, col );
```

---

## 8. Key Classes Quick Reference

| Class                    | Singleton? | Main Responsibility |
|--------------------------|------------|---------------------|
| `ProcessorFactory`       | Yes        | Type registry |
| `TranslationDatabase`    | Yes        | Text mapping + local scope + mismatch |
| `FinalTextProcessor`     | No (per comment+type) | Rules + validation |
| `Config`                 | Yes        | All runtime flags |
| `FileProcessDef`         | No         | One row from defs.csv |
| `EjrefToleranceProcessor`| Yes (via factory) | English reference hacks |
| `EventProcessor`         | No         | Structured event translation |
| `SpecialProcessor`       | No         | One-off hard-coded fixes |

---

## 9. Gotchas and Best Practices

- **Processor order is sacred** — don't change the sequence in `ProcessTranslations()` lightly.
- `evsb` has **two** paths: structured (`EventProcessor`) and fallback (`EvsbProcessor`). The structured path is preferred.
- Always load local scope early if your processor uses per-comment overrides.
- `FinalTextProcessor` can **mutate** the translation (rules) and can skip validation.
- Item processors share state (the same `ItemProcessor` instance is registered for 8 types).
- When adding new structured data, consider whether it should go through `SpecialProcessor` first.
- Backup logic lives in `BackupManager` — always call it before writing in in-situ mode.
- Mismatch logging is global; use `Logger` for per-processor diagnostics.

---

## 10. Related Tools in the Monorepo

- `FFXIDat/` — low-level clean-room parsers (`XiString`, `DMsg`, `ZoneEventImage`, `ZoneActor`, etc.).
- `FFXIDatProcessor/` — database-centric extraction + event DB population. Used by `FFXIDatAdv`.
- `FFXITransAdv/` — heavier event tooling (opcode analysis, etc.).

Many FFXITrans processors lean on FFXIDat classes directly.

---

This manual focuses on the **processor pipeline** and priorities as requested. For full class-level API details, read the headers + the concrete processor implementations.

**Last updated**: based on current source (2026-era state in the repo).