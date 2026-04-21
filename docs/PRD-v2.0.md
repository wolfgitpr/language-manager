# Language Manager 产品需求文档 v2.0

**版本**：2.2  
**日期**：2026-04-21  
**核心目标**：C++17 插件化 G2p（Grapheme-to-Phoneme）框架，遵循"Write Once, Run Forever"设计理念。

---

## 1. 产品概述

Language Manager 是一个模块化语言处理框架。核心库提供文本分割（Splitter）和语言标记（Tagger）的内置实现，通过插件提供语音转换（G2p）、推理驱动（Driver）、字典查询（Dict）等能力。

**设计原则**：
- 简洁可靠：遇错直接返回，不设计重试或回滚
- 接口抽象稳定：Level 锚定 API 结构兼容性，Version 约束内部实现兼容性
- 长期免维护：插件加载后常驻内存，无复杂生命周期管理
- 如无必要不过度设计：避免不必要的抽象层

**支持语言**：普通话（cmn）、粤语（yue）、日语（jpn）、英语（eng）、数字（num）、标点（punc）、未知（unknown）

---

## 2. 核心概念

### 2.1 Level 与 Version

| 概念 | 含义 | 格式 | 校验场景 |
|------|------|------|----------|
| **Level** | Core API 结构版本（函数签名、结构体布局） | 整数（1, 2, 3...） | 管理器与核心插件间、插件间依赖 |
| **Version** | 插件内部实现版本 | MAJOR.MINOR.PATCH | 插件间依赖解析 |

- **Level** 决定上层 API 是否兼容——结构体字段、函数参数变更时递增 Level。
- **Version** 描述同一 Level 下的内部实现差异。例如某 G2p 插件在 Version 1.x 和 2.x 分别兼容不同格式的 ONNX 模型，但上层 API（Level）不变。依赖方可通过版本范围约束（`>=1.0`, `~2.1`, `1.0-2.0`, `*`）选择所需的实现版本。

推荐：Version 首位与 Level 一致（Level=1 → Version=1.x.x）。

**Level 兼容性规则**（核心插件与管理器）：

```
Manager Level = M, Plugin Level = P
兼容条件: M - 1 <= P <= M
```

工具插件（Driver 等）不受此规则限制，通过依赖声明中的 Level + Version 自动分析。

**依赖解析中的双重校验**：

当模块 A 依赖模块 B 时，依赖声明同时指定 `level` 和 `version`：
1. 先按 `level` 过滤候选模块（精确匹配）
2. 再按 `version` 范围过滤（支持 `>=`, `~`, `*`, 连字符范围等语法）
3. 从满足条件的候选中选取最高版本

### 2.2 插件类型

| 类型 | 使用 Core 结构体 | Level 检查 | 示例 |
|------|-----------------|-----------|------|
| **核心插件** | 是（TaskInput/TaskResult） | 是 | G2p, Dict |
| **工具插件** | 否 | 否 | Driver, 辅助工具 |

判断规则：使用 Core 结构体或继承 Task 的都是核心插件。

> **注意**：Splitter 和 Tagger 不再是插件，而是 core 库的内置工具（见 §3.3）。

---

## 3. 架构

### 3.1 分层结构

```
应用层        Manager (单例，高层 API：split/tag/convert)
               ├── 内置      Splitter (单实例，多正则配置驱动)
               ├── 内置      Tagger  (单实例，多语言配置驱动，优先级排序)
               ↓ 继承
管理层        PackageManager (包发现、依赖解析、模块管理)
               ↓ 继承
工厂层        PluginFactory (动态库加载、插件实例化)
               ↓ 加载
插件层        Plugin → TaskPlugin / DriverPlugin
               ↓ 创建
任务层        Task / SessionTask
               ↓ 使用
支持层        Expected<T>, Error, ConfigAccessor, Logging
```

### 3.2 核心组件

**Manager** — 单例，顶层入口。

```cpp
class Manager : public PackageManager {
public:
    static Manager *instance();
    bool initialize(std::string &errMsg);

    // 插件任务
    Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
    Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

    // 内置工具
    std::vector<std::string> split(const std::string &input);
    std::vector<std::string> split(const std::vector<std::string> &input);

    std::vector<TaggerRes> tag(const std::vector<std::string> &input,
                               bool split = false, bool discard = false,
                               const std::vector<std::string> &priorityLanguages = {});

    // G2p 转换（注：裸指针参数为历史接口，后续版本考虑改为值语义）
    std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);
};
```

**Task** — 处理逻辑基类。

```cpp
class Task : public NamedObject {
public:
    // API 兼容性
    virtual int apiLevel() const = 0;

    // 生命周期
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

    // 配置 API
    virtual std::string getConfig() const;
    virtual Expected<void> setConfig(const std::string &config);
    virtual Expected<void> resetToDefault();
    virtual bool isUsingDefaultConfig() const;

protected:
    // 获取依赖模块并校验 Level
    Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;
};
```

**关于 `apiLevel()` 和依赖 Level 的设计**：

- `apiLevel()` 声明本 Task **提供**的 API Level。调用方通过该方法判断能否调用。
- 依赖的 Level 要求声明在 `package.json` 的 per-dependency 字段中（每个依赖独立指定 `level` 和 `version`），由 `DependencyResolver` 在加载期静态校验。
- 运行时 Task 通过 `getObject()` 获取依赖时，框架自动校验对方的 `apiLevel()` 是否满足 package.json 中声明的要求。

> 不在 Task 接口上添加 `minRequiredLevel()` 等方法——一个 Task 可能依赖多个不同 Level 的模块，单一整数无法表达异构需求，且会与 package.json 声明重复。

**SessionTask** — AI 模型驱动任务，管理推理会话。

```cpp
class SessionTask : public Task {
public:
    virtual Expected<void> open(const std::filesystem::path &path, const NO<TaskInitArgs> &args) = 0;
    virtual Expected<void> close() = 0;
    virtual bool isOpen() const = 0;
    virtual int64_t id() const = 0;
};
```

**Plugin** — 插件基类，两种派生：

```cpp
// 任务插件：创建 Task
class TaskPlugin : public Plugin {
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};

// 驱动插件：创建 SessionFactory
class DriverPlugin : public Plugin {
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

**ModuleSpec** — 模块元数据（id、category、apiLevel、config、path、所属 Package）。

**ModuleCategory** — 同类模块的容器（ObjectPool），系统预定义三类：`driver`, `g2p`, `dict`。

**Package** — 插件包（id、version、vendor、modules、dependencies）。

### 3.3 内置工具：Splitter 与 Tagger

Splitter 和 Tagger 不是插件，而是 core 库中由 Manager 持有的**单实例工具**。它们的行为完全由 Package 中的配置 JSON 驱动，初始化时从所有已加载包中收集配置。

#### Splitter

全局唯一实例。初始化时从所有已加载包中收集正则表达式，对输入文本逐层细分。

**配置格式**（在 package.json 的 `splitter` 字段中声明）：

```json
{
  "splitter": {
    "regexes": ["([\\p{Han}])"],
    "order": 100
  }
}
```

- `regexes`：正则表达式数组，每个正则的第一个捕获组为分割单元。一个包可声明多条正则（如标点包同时切分空白、连字符、换行、标点符号）
- `order`：包间加载优先级（数值小的先应用），同一 `order` 的按包加载顺序排列

**工作流程**：
1. 初始化时，按 `order` 排序合并所有包的 `regexes` 为全局有序正则列表
2. `split(text)` 对输入文本依次应用每条正则，逐层细分——每条正则将前一步的段落进一步拆分，正则匹配部分和非匹配部分都保留为独立段落
3. `split(vector)` 重载对已有段落列表继续细分

#### Tagger

全局唯一实例。初始化时从所有已加载包中收集匹配规则，对文本段逐条标注语言。

**配置格式**（在 package.json 的 `taggers` 数组中声明，一个包可包含多个语言的配置）：

```json
{
  "taggers": [
    {
      "language": "cmn",
      "priority": 100,
      "rules": [
        { "type": "dict",  "value": ["pinyin_dict.txt"], "tag": "pinyin" },
        { "type": "regex", "value": ["([\\p{Han}])"],    "tag": "hanzi" }
      ]
    },
    {
      "language": "punc",
      "priority": 900,
      "rules": [
        { "type": "regex", "value": ["([\\s]+)"],   "tag": "space",  "discard": true },
        { "type": "regex", "value": ["([\\p{P}])"], "tag": "punc",   "discard": true }
      ]
    }
  ]
}
```

- `taggers`：数组，每个元素描述一种语言的标注配置
- `language`：标注为哪种语言（如 `"cmn"`, `"eng"`, `"jpn"`, `"punc"`）
- `priority`：匹配优先级（数值小的先匹配），可被 `tag()` 的 `priorityLanguages` 参数覆盖
- `rules`：匹配规则数组，三种类型：
  - `"regex"`：正则全匹配（`RE2::FullMatch`），`value` 中多个正则用 `|` 合并
  - `"array"`：字符串集合精确匹配
  - `"dict"`：从制表符分隔文件加载词表，同 array 匹配逻辑
- `tag`：匹配后赋予的标签
- `discard`：可选，默认 `false`，标记为 `true` 的段落在最终结果中移除

**工作流程**：
1. 初始化时，按 `priority` 排序收集所有包的 tagger 配置
2. `tag(segments, split, discard, priorityLanguages)` 对每个段落，按优先级顺序逐个尝试匹配——仅对 `language == "unknown"` 的段落生效，已标注的跳过
3. 若传入 `priorityLanguages`，对应语言的 tagger 提升到最高优先级
4. 若 `split == true`，先调用内置 Splitter 切分输入
5. 若 `discard == true`，从结果中移除 `discard` 标记为 `true` 的段落

### 3.4 关键数据结构

```cpp
struct G2pInput {
    std::string lyric;    // 输入文本
    std::string g2pId;    // 使用的 G2p 模块 ID
};

struct G2pRes {
    std::string lyric;                       // 输入文本
    std::string g2pId;                       // G2p 模块 ID
    std::string pronunciation = lyric;       // 发音结果
    std::vector<std::string> candidates;     // 候选发音
    std::string mode = "copy";               // "copy" 或 "convert"
};

struct TaggerRes {
    std::string lyric;                       // 输入文本
    std::string language = "unknown";        // 语言 ID
    std::string tag = "unknown";             // 标签类型
    bool discard = false;                    // 是否丢弃
};
```

### 3.5 多版本任务支持

通过 `VersionedTaskManager<T>` 模板支持同一 Task 的多个 Level 实现：

```cpp
// 每个版本实现 VersionedTaskImplBase
class V1::TaskImpl : public VersionedTaskImplBase { ... };
class V2::TaskImpl : public VersionedTaskImplBase { ... };

// Task 类内部委托给对应版本
class MyTask : public Task {
    VersionedTaskManager<MyTask> _manager;
};
```

---

## 4. Package 格式

Package 是可分发的最小单位，扩展名 `.lmpk`（UTF-8 编码 ZIP）。

```
my-package.lmpk
├── package.json           # 包描述文件
├── modules/               # 模块配置
│   └── my-module/
│       └── config.json
└── assets/                # 资源文件（模型、字典等）
```

### 4.1 package.json

```json
{
  "packageId": "cmn-official",
  "version": "1.0.1",
  "vendor": { "_": "OpenVPI", "zh": "OpenVPI 团队" },

  "splitter": {
    "regexes": ["([\\p{Han}])"],
    "order": 100
  },

  "taggers": [
    {
      "language": "cmn",
      "priority": 100,
      "rules": [
        { "type": "dict",  "value": ["ds-zh-pinyin-lite.txt"], "tag": "pinyin" },
        { "type": "regex", "value": ["([\\p{Han}])"],          "tag": "hanzi" }
      ]
    }
  ],

  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-cmn",
        "class": "g2p.mandarin.MandarinG2pInference",
        "configuration": "modules/g2p-cmn/config.json",
        "dependencies": []
      }
    ]
  }
}
```

**必选**：`packageId`（禁止 `/\[]:;'"` 字符）  
**可选**：`version`, `vendor`, `copyright`, `description`, `url`, `splitter`, `taggers`, `modules`

- `splitter`：内置 Splitter 配置（对象），Manager 初始化时自动收集
- `taggers`：内置 Tagger 配置（数组），一个包可声明多个语言的标注规则
- `modules`：插件模块声明（g2p / driver / dict）

声明文件中的相对路径基于该文件所在目录。

---

## 5. 错误处理

### 5.1 Error

```cpp
class Error {
public:
    enum Type {
        Success = 0, ConfigError, FileSystemError,
        DependencyError, RuntimeError, NotImplementedError, InitializationError
    };

    Error(int type, std::string msg);
    Error(int type, std::string msg, std::string suggestion);

    int type() const;
    bool ok() const;
    const std::string &message() const;
    const std::string &suggestion() const;
};
```

### 5.2 Expected\<T\>

替代异常的错误处理包装器：

```cpp
Expected<std::string> result = someOperation();
if (!result) {
    auto err = result.takeError();
    LOG_ERROR("Failed: {}", err.message());
    return err;  // 直接传播，不重试，不回滚
}
auto value = result.get();
```

---

## 6. 配置管理

### 6.1 ConfigAccessor

```cpp
auto cfg = LangCore::config(spec());

// 必需字段 — 返回 Expected<T>，缺失即报错
auto path = cfg.getPath("model_path");

// 可选字段 — 提供默认值
auto threshold = cfg.getDouble("threshold", 0.5);
auto enabled = cfg.getBool("enabled", true);
```

### 6.2 配置持久化

- **加载**：优先用户配置（`~/.config/language-manager/[taskId]/config.json`），回退默认配置
- **保存**：`setConfig()` 自动写入用户配置目录
- **重置**：`resetToDefault()` 删除用户配置文件

---

## 7. 日志

```cpp
#include <LangCore/Support/Logging.h>

LOG_INFO("Task initialized: {}", taskId);
LOG_ERROR("Config missing key: {}", key);
```

级别：Trace, Debug, Info, Success, Warning, Critical, Fatal。

---

## 8. 初始化流程

```
Manager::initialize()
  → PackageManager::loadPackagesInOrder()
    → 扫描包目录，解析 package.json
    → 收集 splitter/taggers 配置，初始化内置工具实例
    → 收集 ModuleMetadata，构建依赖图
    → 按拓扑序加载插件：PluginFactory::loadPlugin() → Plugin::createTask() → Task::initialize()
```

运行时调用：

```
Manager::split(text)                            → 内置 Splitter 逐层正则切分 → vector<string>
Manager::tag(segments, split, discard, priority) → 内置 Tagger 按优先级逐条匹配 → vector<TaggerRes>
Manager::convert(input)                          → 分发到对应 G2p Task → vector<G2pRes>
```

---

## 9. 依赖系统

依赖在 `package.json` 模块的 `dependencies` 数组中声明：

```json
{
  "packageId": "eng-official",
  "moduleId": "g2p-lstm-eng",
  "level": 2,
  "version": ">=1.0"
}
```

| 字段 | 作用 |
|------|------|
| `packageId` + `moduleId` | 定位候选模块 |
| `level` | 精确匹配 API 结构版本（-1 表示跟随请求方 Level） |
| `version` | 版本范围约束，支持 `*`, `>=x.y`, `~x.y.z`, `x.y-a.b` 等语法 |

**解析流程**：

1. **VersionResolver**：按 packageId/moduleId 过滤 → level 精确匹配 → version 范围过滤 → 选最高版本
2. **DependencyResolver**：迭代解析所有模块依赖（最多 2N 轮），检测缺失和循环依赖
3. **DependencyGraph**：构建有向依赖图，Tarjan 算法检测环，计算拓扑初始化顺序
4. **LevelCompatibilityChecker**：系统级校验核心插件 Level 落在 `[minimumLevel, maximumLevel]` 范围内

---

## 10. 测试

全流程集成测试覆盖以下环节：

| 环节 | 测试内容 |
|------|---------|
| 加载 | 包发现、package.json 解析、插件动态加载 |
| 依赖解析 | Level/Version 校验、循环依赖检测、拓扑排序 |
| Splitter | 多语言混合文本切分、边界情况（空串、特殊字符） |
| Tagger | 多语言标注、优先级覆盖、discard 过滤 |
| G2p | 各语言 G2p 转换正确性、批量处理 |
| 配置 | getConfig/setConfig/resetToDefault 持久化 |
| 错误处理 | 缺失依赖、无效配置、不兼容 Level 的错误报告 |

---

## 11. 目录结构

```
core/
  include/LangCore/        公共头文件
    Base/                   NamedObject, ObjectPool, LangCommon
    Support/                Error, Expected, ConfigAccessor, Logging, DisplayText
    Core/                   Plugin, PluginFactory, PackageManager, Manager
    Task/                   Task, SessionTask, TaskPlugin, SessionFactory, VersionedTaskManager
    Module/                 ModuleSpec, ModuleCategory, DependencyGraph
    Package/                Package
  lib/                      实现（含内置 Splitter/Tagger）

plugins/
  G2ps/
    MandarinG2p/            普通话 G2p（cpp-pinyin）
    CantoneseG2p/           粤语 G2p（cpp-kana）
    LstmG2p/                LSTM 模型 G2p（ONNX）
    ChainG2p/               责任链 G2p 框架
  Dicts/DsDict/             字典查询
  Drivers/OnnxDriver/       ONNX Runtime 推理驱动
  Utils/                    辅助工具（InferUtil, OnnxUtil, Common）

res/G2pPackages/            语言包资源（含 splitter/tagger 配置）
tests/                      全流程集成测试
```

---

## 12. 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 插件类 | `[Name]Plugin` | `MandarinG2pPlugin` |
| 任务类 | `[Name]Task` | `MandarinG2pTask` |
| 命名空间 | `LangPlugins::[Name]` | `LangPlugins::MandarinG2p` |
| 插件 key | `category.plugin-name` | `g2p.mandarin` |
| 插件导出 | `LANGCORE_EXPORT_PLUGIN(Class)` | `LANGCORE_EXPORT_PLUGIN(MandarinG2pPlugin)` |

---

## 13. 依赖项

- **构建**：CMake 3.19+, C++17, qmsetup, vcpkg
- **运行时**：ONNX Runtime, cpp-pinyin, cpp-kana, RE2
- **测试**：Qt 6（测试基础设施）

---

**文档版本**: 2.2  
**最后更新**: 2026-04-21
