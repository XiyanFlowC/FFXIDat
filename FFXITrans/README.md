# FFXITrans - FFXI 汉化文本插入工具

## 简介

FFXITrans 是一个面向《最终幻想XI》的文本提取与汉化插入工具。它可以从游戏数据文件中导出待翻译文本，也可以将已经准备好的翻译重新写回游戏 DAT 文件，用于界面、系统文本、任务文本和部分数据表的中文化。

## 功能特性
- 支持多种 FFXI 数据格式：`XiString`、`DMsg`、`EventStringBase`、`StatusData`、`ItemData`、`MonBridge`、`RecordsOfEminence`
- 支持 `prepare` 模式导出待翻译源数据
- 自动备份原始游戏文件
- 支持原位修改或输出到独立目录
- 简体中文到Shift-JIS编码转换
- 失配文本统计和记录
- 灵活的翻译配置系统
- 支持 `text/src` / `text/tgt` 目录结构覆盖基础文本库
- 支持 `dmsg` 及部分结构化数据的 Cell 级别精细翻译控制

## 支持的文件类型

| 类型 | 说明 | 文件内容 |
|------|------|----------|
| `xis` | XiString | 菜单字符串、界面文本 |
| `dmsg` | DMsg | 对话消息、系统提示 |
| `evsb` | EventStringBase | 事件字符串（有配对 evev 时自动启用 EventProcessor 事件感知翻译） |
| `sd` | StatusData | 状态效果描述 |
| `iab` | ItemData Armour | 防具数据（名称、描述） |
| `iwb` | ItemData Weapon | 武器数据（名称、描述） |
| `iub` | ItemData Usable | 可使用物品数据 |
| `inb` | ItemData Normal | 普通物品数据 |
| `ipb` | ItemData Puppet | 人偶用装备 |
| `isb` | ItemData Slip | 莫古寄存存单 |
| `icb` | ItemData Currency | 货币/票券等项目 |
| `iib` | ItemData Instinct | Instinct 相关项目 |
| `fp` | FixedPhrase | 定型文辞书 |
| `mbd` | MonBridge | Monstrosity / Monipulator 显示名称 |
| `erq` | RecordsOfEminence Quest | 目标任务名称、描述、备注 |
| `erc` | RecordsOfEminence Category | 目标分类名称 |

> `prepare` 模式还会兼容处理部分旧定义中的类型，例如 `mb`。

## 系统要求

- **操作系统** - Windows 7 或更高版本
- **编译器** - 支持 C++20 的 Visual Studio 2019 及以上
- **游戏** - 《最终幻想XI》完整安装版
- **权限** - 需要管理员权限（用于读取注册表和修改游戏文件）

## 构建项目

### 前置条件
- Visual Studio 2019 或更高版本
- Windows SDK
- C++20 工作负载

### 构建步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/XiyanFlowC/FFXIDat.git
   cd FFXIDat
   ```

2. **打开解决方案**
   ```bash
   # 使用 Visual Studio 打开项目
   start FFXIDat.sln
   ```

3. **编译项目**
   - 在 Visual Studio 中按 `Ctrl+Shift+B` 编译整个解决方案
   - 或选择具体的项目编译

4. **输出文件**
   - 编译后的可执行文件位于 `bin\Release` 或 `bin\Debug` 目录

## 使用方法

### 1. 环境准备

- 确保已安装《最终幻想XI》和 POL
- 将 FFXITrans 或其源文件放置在任意目录
- 准备好翻译文本文件
- 以管理员权限运行程序

### 2. 配置文件

#### defs.csv - 文件定义

定义要处理的游戏文件，格式：
```
路径,类型,语言,注释[,Cell索引(可选)]
```

**字段说明：**
- **路径** - 游戏文件在 ROM 目录中的路径（如 `ROM/97/8`）
- **类型** - 文件类型（xis、dmsg、evsb、sd、iab、iwb 等）
- **语言** - 语言代码，当前版本请使用 `jp` 或 `en`
- **注释** - 文件的逻辑标识；任务 CSV、参考匹配、排除规则等都依赖该字段
- **Cell索引** - 指定要处理的列，多个索引用 `|` 分隔（可选）

**示例配置：**
```csv
ROM/120/77,xis,jp,Menu Strings
ROM/118/44,dmsg,jp,Dialog Messages
ROM/176/46,dmsg,jp,sys/qst/sd,2|3
ROM/175/22,dmsg,jp,Battle Messages,1
ROM/119/7,evsb,jp,Event Strings
ROM/76/14,sd,jp,Status Data
ROM/5/8,iab,jp,Item Armour Data
ROM/6/8,iwb,jp,Item Weapon Data
ROM/7/8,iub,jp,Item Usable Data
ROM/4/8,inb,jp,Item Normal Data
```

#### 翻译文本文件

FFXITrans 目前支持两套翻译来源：基础行对行文本库，以及目录化源文本覆盖。

##### 基础文本库

基础翻译文本按行对应原文本，支持多个文本集合：

- **text.txt** - 原始文本（UTF-8 编码，转义格式）
- **text_translated.txt** - 翻译文本（UTF-8 编码，与原文对应行）
- **text{1-n}.txt** - 原始文本增补（按顺序递增）
- **text{1-n}_translated.txt** - 增补的对应翻译

##### 目录化源文本覆盖

如果存在 `text/src` 和 `text/tgt`，程序会递归加载其中同名相对路径的文件，并将其作为对基础文本库的额外覆盖：

- `text/src/...` - 原文文件
- `text/tgt/...` - 对应译文文件

这套结构适合把 `prepare` 导出的结果按文件分类整理后再逐步翻译。

**文本格式示例：**

原始文本 (text.txt)：
```
原始文本示例\n换行符会被转义
原始文本示例2
第三行文本
```

翻译文本 (text_translated.txt)：
```
对应的翻译文本\n翻译内容
翻译内容2
第三行的翻译
```

**特殊字符转义：**
- `\n` - 换行符
- `\r` - 回车符
- `\\` - 反斜杠
- `\t` - 制表符

### 3. 运行程序

#### 命令行模式

```text
FFXITrans.exe
FFXITrans.exe insitu
FFXITrans.exe prepare
```

- `FFXITrans.exe` - 常规模式；读取翻译并写出结果
- `FFXITrans.exe insitu` - 强制原位修改游戏目录中的 DAT 文件
- `FFXITrans.exe prepare` - 导出待翻译源数据到 `text/src_`

#### 标准工作流

1. **启动程序**
   ```
   FFXITrans.exe
   ```

2. **备份选项**（如果存在先前的备份）
   - 选择是否恢复到之前的备份
   - 选择"否"将丢弃现有备份

3. **处理模式选择**
   - **原位修改** - 直接修改游戏文件（会自动备份原始文件）
   - **输出到 output** - 将处理后的文件保存到 output 目录（推荐首次使用）

4. **自动处理流程**
   - 读取 defs.csv 中定义的文件
   - 加载翻译文本
   - 如果存在 `text/src` / `text/tgt`，会额外加载目录化覆盖文本
   - 应用翻译到游戏文件
   - 生成处理报告并显示统计信息

#### `prepare` 工作流

`prepare` 模式用于从游戏 DAT 中提取待翻译内容，输出到程序目录下的 `text/src_`。

```text
FFXITrans.exe prepare
```

行为说明：

- 自动读取 `defs.csv`
- 仅导出 `jp` 定义项
- 事件文本通常导出为 `.txt`
- 物品、任务、定型文等结构化数据会导出为 `.csv`
- 输出目录不存在时会自动创建

建议流程：

1. 运行 `prepare` 生成 `text/src_`
2. 按需要整理为 `text/src` 与 `text/tgt`
3. 完成翻译后运行 `FFXITrans.exe` 或 `FFXITrans.exe insitu`

#### 处理结果

程序会输出以下信息：
- **成功翻译** - 成功替换的文本数量
- **失配统计** - 未找到翻译的原文数量
- **处理进度** - 实时显示正在处理的文件
- **text_mismatch.txt** - 失配文本的详细记录

### 4. 特殊功能

#### dmsg Cell指定翻译

对于 dmsg 类型文件，可以在 defs.csv 中指定只翻译特定的列（Cell），实现精细控制：

```csv
# 翻译所有列（默认行为）
ROM/118/44,dmsg,jp,Dialog Messages

# 只翻译第 2 列和第 3 列
ROM/176/46,dmsg,jp,sys/qst/sd,2|3

# 只翻译第 1 列
ROM/175/22,dmsg,jp,Battle Messages,1

# 翻译第 1、3、5 列
ROM/200/50,dmsg,jp,Multi Column,1|3|5
```

**Cell索引规则：**
- 索引从 1 开始（不是 0）
- 多个索引用 `|` 分隔
- 无效索引自动忽略
- 超出范围的索引不会报错
- 只对字符串类型 Cell 生效，数值 Cell 被忽略

除 `dmsg` 外，部分结构化类型在当前实现中也会使用该字段进行列级控制。

## 文件结构

### 基本目录结构

```
FFXITrans/
├── FFXITrans.exe      # 主程序
├── defs.csv           # 文件定义配置
├── text.txt           # 原始文本
├── text_translated.txt # 翻译文本
├── text1.txt          # 原始文本1（若有）
├── text1_translated.txt # 翻译文本1（若有）
├── cp932.csv          # 代码页转换表
├── chs2sjis.csv       # 中文转Shift-JIS映射
├── text/
│   ├── src/           # 目录化原文
│   ├── tgt/           # 目录化译文
│   └── src_/          # prepare 输出目录
├── backup/            # 备份目录（自动创建）
│   └── ROM/...        # 原始文件备份
├── output/            # 输出目录（可选）
│   └── ROM/...        # 处理后的文件
└── text_mismatch.txt  # 失配文本记录（自动生成）
```

### 游戏文件路径

程序会自动从 Windows 注册表读取游戏安装路径。当前实现会依次尝试以下位置：
```
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\PlayOnline\InstallFolder
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\PlayOnlineEU\InstallFolder
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\PlayOnlineUS\InstallFolder
```

## 高级配置

### config.ini - 程序配置文件

`config.ini` 是可选的配置文件，用于在程序启动时自动设置各种选项，避免每次都需要手动交互。

**文件位置** - 放置在 FFXITrans.exe 所在目录

**配置格式** - INI 格式（key=value），支持注释和空格

#### 配置选项

| 选项 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `game_path` | 路径 | 游戏安装目录（覆盖自动检测的路径） | `game_path=C:\Program Files (x86)\PlayOnline\SquareEnix\FFXI` |
| `in_situ` | 布尔值 | 是否原位修改游戏文件，而非输出到 output 目录 | `in_situ=true` |
| `english_mode` | 布尔值 | 强制英文模式，仅处理 PlayOnlineEU/US 安装 | `english_mode=false` |
| `output_path` | 路径 | 自定义输出目录（仅在 in_situ=false 时有效） | `output_path=./output` |
| `no_mismatch_log` | 布尔值 | 不生成 `text_mismatch.txt` | `no_mismatch_log=true` |
| `en_as_ja` | 布尔值 | 英文模式下优先按 JP 参考文本做映射 | `en_as_ja=true` |
| `ejref_tolerance` | 布尔值 | 启用更宽松的 EJ 参考匹配处理 | `ejref_tolerance=true` |
| `verbose` | 布尔值 | 输出更详细的处理日志 | `verbose=true` |
| `noname` | 布尔值 | 跳过名称字段，只处理描述等文本 | `noname=true` |
| `babel` | 字符串 | 控制是否在译文前附带原文；支持 `false`、`bilingual`、`exotic`、`trilingual` 等值 | `babel=trilingual` |
| `bilingual` | 布尔值 | 旧版兼容配置，等价于启用当前语言原文前缀 | `bilingual=true` |
| `excludes` | 列表 | 按 `comment` 排除定义项，支持 `,` 分隔和通配符 | `excludes=ev/*,sys/debug_*` |
| `ctrl_seq_check` | 字符串 | 控制序列检查模式；`off`/`skip`/`strict` | `ctrl_seq_check=strict` |
| `sys_job_workaround` | 布尔值 | 启用职业名特殊处理（武士/武僧缩略等） | `sys_job_workaround=true` |

**布尔值格式** - 支持以下任意一种：
- 数字：`1`（真）或 `0`（假）
- 单词：`true`/`false`、`yes`/`no`
- 区分大小写：`True`、`FALSE` 等不可识别

**注释** - 使用 `;` 或 `#` 开头
```ini
; 这是一个注释
# 这也是注释
```

#### 配置示例

**示例 1 - 基础配置（推荐新手）**
```ini
; FFXITrans 配置文件
; 这个配置将输出到 output 目录，保证安全

; 游戏路径（注释掉则自动检测）
; game_path=C:\Program Files (x86)\PlayOnline\SquareEnix\FFXI

; 输出到 output 目录而非原位修改
in_situ=false

; 英文模式（如果安装的是 PlayOnlineEU/US）
english_mode=false
```

**示例 2 - 原位修改配置（仅限有备份的情况）**
```ini
; 直接修改游戏文件
in_situ=true

; 禁用英文模式
english_mode=false
```

**示例 3 - 自定义输出路径**
```ini
; 输出到自定义目录
in_situ=false
output_path=D:\FFXI_Translations\output

; 游戏路径
game_path=C:\Program Files (x86)\PlayOnline\SquareEnix\FFXI
```

#### 读取优先级

程序按以下顺序应用配置：

1. **注册表或自动检测结果** - 提供默认游戏路径与语言模式
2. **config.ini** - 覆盖默认设置
3. **命令行参数** - `prepare` 进入导出模式，`insitu` 强制原位输出
4. **用户交互** - 仅在仍需确认输出方式时使用

因此，使用 `config.ini` 可以显著减少交互；使用 `insitu` 则可直接强制原位写出。

#### 运行模式

- **无参数 + 无 config.ini** - 交互模式（询问用户选择）
- **无参数 + 有 config.ini** - 自动模式（按配置运行）
- **命令行 `insitu` 参数** - 强制原位修改模式，忽略 config.ini 的 `in_situ` 设置
- **命令行 `prepare` 参数** - 导出待翻译源数据，不执行插入流程

#### 配置文件示例完整版本

```ini
; ============================================
; FFXITrans 配置文件
; ============================================

; 游戏路径设置
; 说明：指定 FFXI 游戏的安装目录
; 默认：从注册表自动检测
; game_path=C:\Program Files (x86)\PlayOnline\SquareEnix\FFXI

; 原位修改设置
; 说明：true=直接修改游戏文件（需备份）
;false=输出到 output 目录（推荐新手）
; 默认：false（询问用户）
in_situ=false

; 英文模式
; 说明：true=仅处理 PlayOnlineEU/US 安装的英文文件
;      false=处理日文文件
; 默认：根据注册表自动检测
english_mode=false

; 输出路径设置
; 说明：当 in_situ=false 时，指定输出目录
; 默认：./output（当前目录的 output 文件夹）
; output_path=./output

; 关闭失配日志
; no_mismatch_log=false

; 输出详细日志
; verbose=false

; 旧版/兼容项：跳过名称翻译
; noname=false

; 在译文前附加原文
; 可选：false / bilingual / exotic / trilingual
; babel=false

; 排除指定 comment，支持通配符
; excludes=ev/*,sys/debug_*
```

### 编码转换表

#### cp932.csv - CP932 编码映射
定义 Windows CP932 编码的字符映射规则，用于特殊字符的处理。

#### chs2sjis.csv - 中文到 Shift-JIS 映射
定义简体中文字符到日文 Shift-JIS 编码的映射关系。格式通常为：
```
CJK字符,对应的Shift-JIS字符或组合
```

### 文本格式处理

程序使用转义格式处理特殊字符以保证跨平台兼容性：

| 转义符 | 含义 |
|--------|------|
| `\n` | 换行符 (LF) |
| `\r` | 回车符 (CR) |
| `\t` | 制表符 (TAB) |
| `\\` | 反斜杠 |

**注意** - 转义符必须在文本中明确写出，而不是直接使用实际字符。

## 故障排除

### 常见问题

#### 1. "未能正确读取注册表信息"

**原因** - 程序无法从注册表读取 FFXI 安装路径

**解决方案**
- 确认 FFXI 和 POL 已正确安装
- 以管理员权限运行程序
- 检查 `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\PlayOnline\InstallFolder` 是否存在
- 如果是英文客户端，也检查 `...\PlayOnlineEU\InstallFolder` 或 `...\PlayOnlineUS\InstallFolder`

#### 2. "翻译文件和原文文件的行数不一致"

**原因** - `text.txt` 和 `text_translated.txt` 的行数不匹配，或 `text/src` / `text/tgt` 中同名文件内容未对应

**解决方案**
- 检查两个文件的行数是否完全相同
- 确认文件编码均为 UTF-8
- 检查是否有空行或额外的换行符
- 确保文件末尾没有多余空白行
- 如果使用目录化覆盖，确认 `text/src` 与 `text/tgt` 具有一致的相对路径结构

#### 3. 翻译效果不生效

**原因** - 多种可能，最常见的是配置或文本格式错误

**解决方案**
- 检查 defs.csv 中的文件路径是否正确（使用 ROM 相对路径）
- 确认 defs.csv 的文件格式正确（逗号分隔）
- 验证翻译文本与原文的对应关系
- 查看 text_mismatch.txt 了解失配情况
- 检查是否选择了正确的处理模式（原位修改 vs 输出到 output）
- 确认游戏文件没有被游戏客户端锁定

#### 4. 程序崩溃或卡死

**原因** - 游戏文件被占用或数据格式不兼容

**解决方案**
- 完全关闭 FFXI 游戏客户端和 POL
- 关闭所有可能访问游戏文件的其他程序
- 尝试先使用"输出到 output"模式测试
- 检查备份文件是否完整，必要时手动恢复

#### 5. 编码错误或乱码

**原因** - 文本编码不统一

**解决方案**
- 确保 text.txt 和 text_translated.txt 都用 UTF-8 编码保存
- 检查 chs2sjis.csv 映射表是否包含所需字符
- 某些特殊符号可能不支持，查看 text_mismatch.txt 获取详情

### 调试信息

程序会生成以下输出文件帮助诊断问题：

| 文件 | 用途 |
|------|------|
| `text_mismatch.txt` | 记录所有未找到翻译的原始文本 |
| `text/src_/...` | `prepare` 导出的待翻译文本/CSV |
| `backup/ROM/...` | 处理前的原始文件备份 |
| `output/ROM/...` | 处理后的输出文件（仅在"输出到 output"模式） |

## 安全指南

### 备份策略

- 程序会自动备份修改的文件到 `backup` 目录
- 每次处理都会更新备份（不保留历史版本）
- 建议在处理重要文件前手动备份整个 ROM 目录

### 最佳实践

1. **首次使用** - 选择"输出到 output"模式进行测试
2. **验证结果** - 确认翻译效果满意后再进行原位修改
3. **备份游戏** - 处理前手动备份整个游戏目录
4. **关闭游戏** - 处理前确保游戏客户端完全关闭
5. **单次处理** - 避免同时运行多个 FFXITrans 实例

### 恢复原始文件

如需恢复原始文件：

1. **从备份恢复**
   - 程序启动时会提示恢复备份
   - 选择"是"即可恢复

2. **手动恢复**
   - 将 `backup/ROM/` 中的文件复制回原位置

## 更新日志

### 当前版本
- 支持 `EventProcessor`：当 evsb 有配对 evev 时，利用 FFXIDat 的 `ZoneEventImage`/`ZoneActor` 解析事件结构，从 `text/tgt/event/` 读取译文建立逐事件补丁，未覆盖索引自动回退 `TranslationDatabase`
- 支持 `TranslationDatabase` 本地作用域：每个文件处理时优先匹配 `text/{src|tgt}/<comment>.txt`，实现文件级翻译优先

### v0.31
- 支持 `prepare` 源数据导出
- 支持 `text/src` / `text/tgt` 目录化覆盖
- 支持 `MonBridge`、`RecordsOfEminence` 等更多类型
- 支持更细粒度的配置项与排除规则

## 技术细节

### 支持的编码

- **输入文本** - UTF-8
- **游戏文件** - Shift-JIS（日文）/ CP932
- **配置文件** - UTF-8 或 ANSI

### 数据格式

FFXITrans 使用 FFXIDat 库解析以下格式：
- **XiString** - 字符串表
- **DMsg** - 结构化消息表（支持多列）
- **EventStringBase** - 事件文本
- **ItemData** - 物品信息
- **StatusData** - 状态效果描述
- **MonBridge** - 特殊名称表
- **RecordsOfEminence** - 目标任务/分类数据

### 内存要求

- **最小** - 512 MB RAM
- **推荐** - 2 GB RAM 或以上

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。

## 技术支持

### 获取帮助

如遇到问题，请按以下顺序检查：

1. **文件完整性** - 检查游戏文件是否完整
2. **配置正确性** - 验证 defs.csv 和文本文件格式
3. **权限问题** - 确认以管理员身份运行
4. **编码问题** - 检查文本文件编码
5. **依赖项** - 确认所有必需的库和文件都已安装

### 报告问题

如遇到 Bug 或有功能建议，请在 GitHub 上提交 Issue：
- 项目主页 - https://github.com/XiyanFlowC/FFXIDat

### 联系方式

- **GitHub** - XiyanFlowC
- **项目主页** - https://github.com/XiyanFlowC/FFXIDat

## 相关资源

- [FFXIDat GitHub](https://github.com/XiyanFlowC/FFXIDat)
- [FFXIAH](https://www.ffxiah.com/)
- [PlayOnline](https://www.playonline.com/)

## 声明
本工具非官方产品，与 Square Enix 无任何关联。使用本工具可能违反游戏的使用条款，请自行承担风险。建议仅用于个人学习和研究目的。

FINAL FANTASY XI 及 PLAYONLINE 等是 Square Enix Co., Ltd. 的商标或注册商标。版权所有 ? SQUARE ENIX CO., LTD.。

---

**最后更新** - 2026-05-11
