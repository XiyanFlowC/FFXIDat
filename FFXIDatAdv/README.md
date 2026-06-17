# FFXIDatAdv

FFXI 事件对话导出工具。以 **Actor**（NPC/实体）为单位，从 FFXI 的事件二进制文件（evev/evac/evsb）中提取对话数据，输出结构化 JSON。

阿你问我为什么不用Python吗因为C++更快啊（逃

## 数据流

```
zone_events.csv → ZoneConfig
      ↓
GamePathResolver → 解析 FFXI 安装路径中的 .DAT 文件
      ↓
EventBinaryDat::Parse(evev)   → ActorBlock[] (原始字节码)
EntityDat::Parse(evac)        → EntityEntry[] (实体名称)
EventStringBase::Read(evsb)   → string[] (本地化文本)
      ↓
EventLinker::LinkZone
      ↓
BytecodeAnalyzer::ExtractDialogues → 反汇编字节码，提取 DialogueLine
      ↓
EventWriter → JSON (zone/{id}/*.json + common/*.json)
```

## 输出结构

```
event/
  index.json                  ← 全量索引
  common/
    index.json                ← 公共 Actor 清单
    Survival Guide.json       ← 跨区域通用的 Actor
    Home Point.json
    ...
  zone/{zone_id}/
    index.json                ← 该区域 Actor 索引
    ActorName.json            ← 私有 Actor 对话数据
```

## 公共 Actor 去重

同名 Actor 出现在多个区域时，验证字节码完全一致，则提升为 "common"（公共 Actor），输出到 `common/` 目录。

## 构建

- Visual Studio 2022 (v143)
- C++20
- 依赖：FFXIDat.lib, xybase.lib（workspace 中的姊妹项目）

## 参考

- [XiEvents](https://github.com/atom0s/XiEvents) — FFXI 事件系统逆向文档
- [FFXI-EventsDump](https://github.com/sruon/FFXI-EventsDump/) — Python 版事件导出工具

## 工具