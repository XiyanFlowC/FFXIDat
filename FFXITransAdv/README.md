# FFXITransAdv

FFXI 事件对话提取/回写工具。基于 `FFXIDatAdv` 的字节码分析引擎和 `FFXITrans` 的翻译管线，支持从 DAT 提取事件对话、回写翻译文本，以及按 Actor 分组的层级化 TXT 输出。

## 文件结构

```
FFXITransAdv/
  ApplicationAdv.cpp   — 主处理流程（PrepareSourceData / ExportOtherTypes / Run）
  EventTextOut.cpp     — 从 evev/evac/evsb 提取对话 → TXT
  EventTextIn.cpp      — 从 TXT 回写到 evsb DAT
  EventFileProcessor.cpp — ZoneRegistry、defs.csv 解析
  EventDefs.h          — 数据结构与语言常量定义
  FFXITransAdv.cpp     — CLI 入口
```

## CLI 用法

```
FFXITransAdv.exe extract [zone]   提取事件对话到 texts/event/
FFXITransAdv.exe apply   [zone]   将翻译后的 TXT 写回 DAT
FFXITransAdv.exe prepare          准备源文本（供翻译用）
FFXITransAdv.exe list              列出所有区域
```

## 输出结构

```
texts/event/
  common/            ← 跨区域通用 Actor（文本去重）
    {actorName}/
      {event_id}.txt
  zone/
    {ev|evx}/{zoneName}/
      {actorName}_{entityId}/
        {event_id}.txt
```

## TXT 格式

```
<sel>選択肢1<lf>選択肢2<->     ← 选择分支
<ins:5:13:82:80:80:80>       ← 动态插入（物品/能力等）
<7F:31>                       ← 特殊指令
@ref event/common/xxx.txt     ← 去重引用
```

## 依赖

- Visual Studio 2022 (v143), C++20
- FFXI DAT 安装目录（通过注册表 `SOFTWARE\WOW6432Node\PlayOnline\InstallFolder` 定位）
- `cp932.csv` — Shift-JIS↔UTF-8 码表（置于 exe 目录）
- `defs.csv` — 区域配置（置于 exe 目录）

## 构建

```bash
msbuild FFXITransAdv.vcxproj /p:Configuration=Release /p:Platform=x64
```

将 `data/defs.csv` 和 `cp932.csv` 复制到 `x64/Release/` 后运行。

## 许可

AGPL-3.0。详见 [LICENSE](LICENSE)。
