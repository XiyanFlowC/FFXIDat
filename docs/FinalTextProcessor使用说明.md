# FinalTextProcessor 最终文本处理器使用说明

## 概述

FinalTextProcessor 是 FFXIDat 翻译工具链中的最终文本处理模块，用于在翻译文本写入 DAT 文件之前进行批量替换、修正和验证。通过配置 CSV 规则文件，可以实现自动化的文本处理。

## 规则文件位置

规则文件存放在程序根目录的 `rules` 文件夹下：

- `rules/common.csv` - 通用规则，应用于所有文件
- `rules/<comment>.csv` - 特定文件规则，仅应用于匹配 comment 的文件

例如：`rules/item_armor_jp.csv` 仅会应用于 comment 为 "item_armor_jp" 的数据文件。

## 规则文件格式

规则文件为 UTF-8 编码的 CSV 文件，包含以下列：

| 列名 | 说明 |
|------|------|
| Command | 指令类型（REP/REPRE/SET/SETNXT） |
| TranslatedPattern | 译文中要匹配的模式 |
| TargetText | 替换目标文本 |
| OriginalPattern | 原文匹配正则表达式（可选） |
| OriginalExcludePattern | 原文排除正则表达式（可选） |
| Occurrence | 出现次数过滤（可选） |

### CSV 格式示例

```csv
Command,TranslatedPattern,TargetText,OriginalPattern,OriginalExcludePattern,Occurrence
REP,错误文本,正确文本,,,
REPRE,"(\d+)个","$1 个",,,
```

**注意：**SET 和 SETNXT 的格式不同（少一列）：
```csv
Command,TargetText,OriginalPattern,OriginalExcludePattern,Occurrence
SET,这是新的译文,^原文内容$,,
SETNXT,下一条的译文,^触发条件$,,
```

## 指令说明

### 1. REP - 简单替换

在译文中查找并替换指定文本（精确匹配）。

**语法：**
```csv
REP,查找文本,替换文本,原文正则,排除正则,出现次数
```

**示例：**
```csv
# 将所有"魔力"替换为"MP"
REP,魔力,MP,,,

# 仅在原文包含"magic"时替换
REP,魔力,MP,magic,,

# 仅替换第1-3次出现
REP,魔力,MP,,,1-3
```

### 2. REPRE - 正则表达式替换

使用正则表达式在译文中进行复杂的模式匹配和替换。

**语法：**
```csv
REPRE,译文正则,替换文本,原文正则,排除正则,出现次数
```

**示例：**
```csv
# 在数字和单位之间添加空格
REPRE,(\d+)(个|只|件),$1 $2,,,

# 使用捕获组重新排列文本
REPRE,"HP:(\d+)，MP:(\d+)",体力: $1 / 魔力: $2,,,
```

### 3. SET - 直接设置译文（新增）

当原文满足条件时，直接将当前条目的译文替换为指定文本，**忽略原有译文**。

**语法：**
```csv
SET,目标译文,原文正则,排除正则,出现次数
```

**注意：**SET 指令不需要 TranslatedPattern（译文匹配模式），因此 CSV 格式比其他指令少一列。

**使用场景：**
- 原文完全错误，需要重新翻译
- 某些特定原文需要固定的译文
- 需要根据原文特征批量设置译文

**示例：**
```csv
# 当原文为"Unknown"时，设置译文为"未知"
SET,未知,^Unknown$,,

# 所有包含"deprecated"的条目设置为"已废弃"
SET,已废弃,deprecated,,

# 第5次出现特定原文时设置特殊译文
SET,【特殊说明】,^Special Case$,,5
```

**注意事项：**
- SET 指令的 CSV 格式为：`Command,TargetText,OriginalPattern,OriginalExcludePattern,Occurrence`（比其他指令少一列）
- TargetText（第二列）即为最终的译文
- 必须配合 OriginalPattern 使用，否则会无条件替换所有文本
- SET 执行后会终止后续规则的处理

### 4. SETNXT - 设置下一条译文（新增）

当原文满足条件时，将**下一条**文本的译文设置为指定内容。

**语法：**
```csv
SETNXT,下一条的目标译文,原文正则,排除正则,出现次数
```

**注意：**SETNXT 指令同样不需要 TranslatedPattern，CSV 格式与 SET 相同。

**使用场景：**
- 根据前一条内容决定后一条的翻译
- 处理上下文相关的翻译序列
- 自动修正已知的翻译顺序错误

**示例：**
```csv
# 当遇到"Title:"时，下一条设置为"标题内容"
SETNXT,标题内容,^Title:$,,

# 多条连续设置
SETNXT,主标题,^Header$,,
SETNXT,副标题,^Subheader$,,
```

**注意事项：**
- SETNXT 只设置标记，不修改当前条目
- TargetText（第二列）为下一条的译文内容
- SETNXT 的 CSV 格式与 SET 相同（比其他指令少一列）
- 下一条文本会完全被替换，包括跳过所有其他规则

## 高级功能

### 原文条件过滤

通过 `OriginalPattern` 和 `OriginalExcludePattern` 可以精确控制规则应用条件：

```csv
# 仅当原文包含"sword"时替换
REP,剑,武器,sword,,

# 原文包含"sword"但不包含"cursed"时替换
REP,剑,武器,sword,cursed,

# 使用正则表达式匹配原文
REP,物品,道具,^Item:\s+\w+$,,
```

### 出现次数控制

通过 `Occurrence` 列可以控制规则仅在特定出现次数时生效：

```csv
# 仅第一次出现时替换
REP,勇者,主角,,,1

# 第2-5次出现时替换
REP,勇者,冒险者,,,2-5

# 第10次及以后替换
REP,勇者,传说中的勇者,,,10-999999

# 多个范围（用|分隔）
REP,魔法,魔术,,,1-3|10-12|20
```

### 规则执行顺序

规则按照 CSV 文件中的顺序执行：
1. 先应用 `common.csv` 中的规则
2. 再应用特定文件的规则
3. 每个文件内按行顺序执行

**示例工作流：**
```csv
# Step 1: SET 可能直接替换整条译文
SET,固定译文,^特定原文$,,

# Step 2: SETNXT 标记下一条
SETNXT,,^触发条件$,,

# Step 3: REP/REPRE 对未被SET替换的文本进行调整
REP,旧词,新词,,,
REPRE,(\d+),【$1】,,,
```

## 实际应用案例

### 案例1：修正游戏内置文本错误

```csv
# 修正翻译错误
SET,正确的物品名称,^Item:\s+WrongName$,,
SET,正确的任务描述,^Quest:\s+12345$,,
```

### 案例2：统一术语翻译

```csv
# 统一"HP"的翻译
REP,生命值,HP,,,
REP,血量,HP,,,
REP,体力,HP,,,
```

### 案例3：格式调整

```csv
# 调整数字格式
REPRE,(\d+)个,数量：$1,,,
REPRE,"等级(\d+)","Lv.$1",,,
```

### 案例4：上下文相关翻译

```csv
# 当遇到"Chapter 1"时，下一条设为章节标题
SETNXT,第一章 起始之地,^Chapter\s+1$,,
SETNXT,第二章 冒险继续,^Chapter\s+2$,,
```

### 案例5：批量处理系列文本

```csv
# 对所有武器描述进行处理
REP,攻击力,ATK,weapon,,
REP,防御力,DEF,armor,,
SET,【已停用】,deprecated,,
```

## 控制序列验证

对于 `evsb` 类型文件，系统会自动验证特殊控制序列：

### Switch 验证
```
原文: 你获得了<switch:1>[苹果/橙子/香蕉]
译文: You got <switch:1>[Apple/Orange/Banana]  ✓ 正确（选项数匹配）
译文: You got <switch:1>[Apple/Orange]         ✗ 错误（选项数不匹配）
```

### Gender 验证
```
原文: <gender>[他/她]很高兴
译文: <gender>[He/She] is happy  ✓ 正确（两个选项）
译文: <gender>[He/She/It]         ✗ 错误（必须恰好两个选项）
```

验证模式可在 `config.ini` 中配置：
- `Off` - 不验证
- `Skip` - 警告但继续（默认）
- `Strict` - 验证失败则中止

## 调试和日志

规则执行信息会记录在 `log.txt` 中：

```
Loaded 15 final text rules from rules/common.csv
Loaded 8 final text rules from rules/item_armor_jp.csv
Final Text Processor 控制序列校验失败：comment=dialog_12, row/id=45
原因：译文中的 <switch:...> 的选项数量不一致。
原文：选择你的<switch:1>[职业/角色/身份]
译文：Choose your <switch:1>[Class/Character]
```

## 常见问题

### Q: 规则不生效怎么办？
A: 检查以下几点：
1. CSV 文件编码是否为 UTF-8
2. 规则文件名是否与 comment 匹配
3. OriginalPattern 是否正确匹配原文
4. 查看 log.txt 中的规则加载信息

### Q: SET 和 SETNXT 的区别？
A: 
- SET 修改**当前条目**的译文
- SETNXT 修改**下一条目**的译文
- SET 会终止后续规则处理，SETNXT 不会

### Q: 正则表达式如何转义特殊字符？
A: 使用反斜杠 `\` 转义：
```csv
# 匹配 "[物品]"
REPRE,\[物品\],【物品】,,,
```

### Q: 如何注释规则？
A: 在 Command 列使用 `#` 开头：
```csv
#,这是注释,,,,
REP,测试,测试内容,,,
```

## 最佳实践

1. **从通用到特殊**：在 common.csv 中放置通用规则，特定文件规则放在对应的 CSV 中
2. **使用注释**：为复杂规则添加注释说明
3. **测试验证**：修改规则后运行翻译，检查 log.txt 确认效果
4. **版本控制**：将 rules 目录纳入版本控制
5. **谨慎使用 SET**：SET 会完全覆盖译文，确保配合正确的原文匹配条件

## 技术参考

- 正则表达式语法：ECMAScript (C++ std::regex)
- CSV 格式：RFC 4180（支持引号转义）
- 字符编码：UTF-8
- 控制序列：FFXI 特定格式（`<switch:N>`, `<gender>`）

