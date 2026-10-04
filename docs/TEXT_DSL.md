# FFXI 文本 DSL：转义与控制序列表示法

## 概述

DAT 里的"文本"是 cp932（Shift-JIS，含 Square Enix 的欧文扩展）字节串，中间夹着不可打印的 FFXI 控制码。为了能在 CSV、SQLite 数据库、翻译文件（`text/`）、`text_mismatch.txt` 和日志里安全地保存和传递这些字节，仓库用一套固定的转义写法加控制序列写法（本文称 DSL）来表示它们。

本文说明：这套表示法怎么写、在哪一层解析、哪些内容必须原样保留、哪些会被重写。

## 1. 转义层：`xybase::string::escape` / `unescape`（`xybase/xystring.h`）

`escape` 默认只转义三类字符（`esc_ascii_ctrl`、`esc_non_ascii` 两个可选参数打开后才会转义其它 ASCII 控制字符和非 ASCII 字符，仓库里的调用点都用默认值）：

| 原字符 | 写法 |
|---|---|
| `\` (0x5C) | `\\` |
| CR (0x0D) | `\r` |
| LF (0x0A) | `\n` |

`unescape` 识别 `\\`、`\r`、`\n`、`\t`、`\xHH`（两位十六进制）、`\uXXXX`（四位十六进制）。无法识别的转义（例如 `\d`）会丢掉反斜杠、只保留后面的字符，所以不要随手在译文里写反斜杠。

两点需要注意：

- 默认的 `escape` **不**转义控制字符，所以 FFXI 的原始控制码（如 0x07、0x13、0x1F）在 CSV 和数据库里通常就是原始字节，而不是文本标签；人工书写时可以用 `\x13` 这种写法。
- 转义层只处理字符，不改变编码：DAT 侧是 cp932，CSV 与数据库侧是 UTF-8。

## 2. 数据流：解析发生在哪一层

```
DAT ──escape──▶ CSV / SQLite / text_mismatch.txt / text/{src,tgt}/*.txt
                     │
                     ├─ 人工翻译、规则文件（REP/REPRE/SET/SETNXT）
                     ▼
DAT ◀──unescape──  译文
```

- 导出/入库：`FFXIDatProcessor`（`SQLiteDataSource.cpp`、`SQLiteDataSource_ROE.cpp`）和 `FFXITrans`（`ProcessorUtils.cpp` 与各 `Processors/*Processor.cpp`）在写入 CSV、数据库、文本文件之前统一调用 `xybase::string::escape`。
- 写回：读出译文后统一调用 `xybase::string::unescape` 再写进 `Record`（例如 `SQLiteDataSource.cpp` 的 `TranslateDat`、`Processors/ItemProcessor.cpp`）。
- 因此：CSV / 数据库 / 翻译文件里的文本一律按本文的 DSL 理解；规则文件里的 `TranslatedPattern`、`OriginalPattern` 也作用在**转义后**的文本上（`FinalTextProcessor::ProcessEscaped` 内部先 `unescape`、处理完再 `escape`）。

## 3. 控制序列的三种形态

1. **原始控制字节**：控制码本身是单字节二进制码（如 `0x07` 换行、`0x13` 物品名插入），转义层默认不把它们改写成标签，因此它们在 CSV/数据库里通常保持原始字节。
2. **事件文本（evsb）的标签形式**：`FFXIDat/EventString.cpp` 的编解码器把控制码与文本标签互相转换，例如 `<lf>`、`<name>`、`<num>`、`<item:...>`、`<ins ...>`。控制码表见 `docs/FILE_FORMATS.md` 的 "Event Strings → Control Sequences" 一节。
3. **FinalTextProcessor 校验的控制序列**：对 `evsb` 译文会检查 `<switch:N>[...]` 的选项数与原文一致、`<gender>[...]` 恰好两个选项。校验模式由 `config.ini` 控制（`Off` / `Skip` / `Strict`）。

## 4. 必须保留的内容

- 控制码与标签的**数量、顺序**；`<switch:N>` 的 N 与选项个数、`<gender>` 的两个选项不能增删。
- 占位符（`<name>`、`<num>`、`<item:...>` 等）必须仍然出现在译文中。
- 换行按原文的方式书写：DAT 侧的换行是控制码，不要在 CSV 里直接敲回车，也不要把它当作普通文本内容。
- 规则匹配的原文是转义后的原文，写 `OriginalPattern` 时要按 DSL 写。

## 5. 会被重写的内容

- 规则命中的部分：`REP` / `REPRE` 只改匹配到的文本，`SET` 覆盖整条译文，`SETNXT` 覆盖下一条的译文。
- 译文里手写的 `\xHH` / `\uXXXX` 会在写回 DAT 时变成真实字节，写错就会破坏控制码。
- 明确不翻译的字段（例如 MonBridge 的内部名 `name`）不会被规则触及。

## 6. `|` 的两种用法（不要混淆）

- `data/defs.csv` / `data/FLIST.csv` 的第 5 列（如 `1|2`）：要翻译的 cell 序号，从 1 开始、用 `|` 分隔，留空表示全部 cell；解析见 `ProcessorUtils::ParseCellIndices`。
- 规则文件 `FinalTextProcessor` 的 `Occurrence` 列（如 `1-3|10-12|20`）：出现次数范围，用 `|` 分隔多个范围；解析见 `docs/FinalTextProcessor使用说明.md`。

## 相关文档

- `docs/FILE_FORMATS.md`：每个 DAT 族的二进制格式、控制码表与记录版本选择
- `docs/FinalTextProcessor使用说明.md`：REP / REPRE / SET / SETNXT 规则语法与校验模式
