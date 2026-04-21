# Language Manager 产品需求文档 v2.0

**版本**：2.4  
**日期**：2026-04-21  
**核心目标**：C++17 插件化 G2p（Grapheme-to-Phoneme）框架，遵循"Write Once, Run Forever"设计理念。

---

## 1. 产品概述

Language Manager 是一个模块化语言处理框架。核心功能（语音转换、推理驱动、字典查询）均通过插件提供，核心库提供统一的插件管理、依赖解析和任务调度机制。

> **关于文本分割与语言标注**：Splitter（文本分割）和 Tagger（语言标注）已移至前端实现，不再属于核心框架。当前仓库中仅在测试代码（`tests/tst_langCore/`）中保留了简化的本地实现，供全流程集成测试使用。

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

---

## 3. 架构

### 3.1 分层结构

```
应用层        Manager (单例，高层 API：convert)
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
    bool initialized() const;

    // 插件任务查询
    Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
    Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

    // G2p 转换
    std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);
};
```

**Task** — 处理逻辑基类。

```cpp
class Task : public NamedObject {
public:
    explicit Task(const ModuleSpec *spec);

    // API 兼容性
    virtual int apiLevel() const = 0;

    // 生命周期
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

    // 元数据访问
    const ModuleSpec *spec() const;
    PackageManager *Mgr() const;

    // 配置 API
    virtual std::string getConfig() const;

protected:
    // 获取依赖模块并校验 Level
    Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;

    // 配置加载（子类在 initialize() 中调用）
    Expected<void> initializeConfig();
    Expected<std::string> loadConfig() const;
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
class Plugin {
public:
    virtual const char *iid() const = 0;
    virtual const char *key() const = 0;
    virtual int apiLevel() const = 0;
    std::filesystem::path path() const;
};

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

**简化宏**——快速定义插件导出：

```cpp
// 定义并导出 TaskPlugin
LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, PluginKey, ApiLevel)

// 定义并导出 DriverPlugin
LANGCORE_DEFINE_DRIVER_PLUGIN(PluginClass, FactoryClass, PluginKey, ApiLevel)
```

**ModuleSpec** — 模块元数据（id、category、className、apiLevel、manifestConfiguration、configuration、path、所属 Package）。

**ModuleCategory** — 同类模块的容器（ObjectPool），系统预定义三类：`driver`, `g2p`, `dict`。通过宏 `LANGCORE_DECLARE_MODULE_CATEGORY` / `LANGCORE_DEFINE_MODULE_CATEGORY` 注册。

**Package** — 插件包（id、version、vendor、modules、dependencies）。

### 3.3 关键数据结构

```cpp
struct G2pInput {
    std::string lyric;    // 输入文本
    std::string g2pId;    // 使用的 G2p 模块 ID
};

enum G2pErrorType {
    NoError = 0,
    InitError, ModelInitFailed, SessionInitFailed, ConfigError,
    InvalidInput, EmptyInput, InvalidLyric, UnsupportedCharacter,
    ResourceError, ModelNotFound, DictNotFound, VocabNotFound,
    ConversionError, PinyinConversionFailed, ModelInferenceFailed,
    PhonemeGenerationFailed, DependencyError, RuntimeError,
    TensorError, SessionError, UnknownError,
};

struct G2pRes {
    std::string lyric;                       // 输入文本
    std::string g2pId;                       // G2p 模块 ID
    std::string pronunciation;               // 发音结果（默认为 lyric）
    std::vector<std::string> candidates;     // 候选发音
    std::string mode = "copy";               // "copy" 或 "convert"
    G2pErrorType errorType = NoError;        // 错误类型
};

struct TaggerRes {
    std::string lyric;                       // 输入文本
    std::string language = "unknown";        // 语言 ID
    std::string tag = "unknown";             // 标签类型
    bool discard = false;                    // 是否丢弃
};
```

> `TaggerRes` 仍保留在核心数据结构中，供前端或测试使用。

### 3.4 版本化任务 I/O 类型

每个模块类别定义版本化的输入/输出类型，均继承自 `TaskInput` / `TaskResult`：

```cpp
// G2p
class G2pInputV1 : public TaskInput {
    std::vector<std::string> g2pInput;
};
class G2pResultV1 : public TaskResult {
    std::vector<G2pRes> g2pResult;
    std::string errorMessage;
};

// Dict
class DictInputV1 : public TaskInput {
    std::string dictId;
    std::vector<std::string> keys;
    std::string defaultValue;
    uint32_t flags = 0;
};
class DictResV1 : public TaskResult {
    std::vector<std::string> values;
    bool found = false;
    size_t foundCount = 0;
};

// Session（AI 模型推理）
class SessionStartInput : public TaskInput {
    std::map<std::string, NO<ITensor>> inputs;
    std::set<std::string> outputs;
};
class SessionResult : public TaskResult {
    std::map<std::string, NO<ITensor>> outputs;
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

`TASK_IMPLEMENT` 宏可自动生成 Task 的构造函数、`apiLevel()`、`initialize()`、`start()`、`getConfig()` 委托代码：

```cpp
TASK_IMPLEMENT(MyTask, MyTask, Internal::V1, TaskImpl)
```

### 3.6 SessionFactory

为 AI 模型推理提供会话工厂：

```cpp
class SessionFactory : public NamedObject {
public:
    virtual std::string arch() const = 0;
    virtual std::string backend() const = 0;
    virtual Expected<void> initialize(const NO<TaskInitArgs> &args) = 0;
    virtual NO<SessionTask> createSession() = 0;
};
```

相关初始化参数类型：

```cpp
class DriverInitArgs : public TaskInitArgs {
    bool loadFromProcess = false;
    ExecutionProvider ep = CPUExecutionProvider;
    int deviceIndex = -1;
    std::filesystem::path runtimePath;
};

class SessionOpenArgs : public TaskInitArgs {
    bool useCpu = false;
};

enum ExecutionProvider {
    CPUExecutionProvider, CUDAExecutionProvider,
    DMLExecutionProvider, CoreMLExecutionProvider,
};
```

---

## 4. Package 格式

Package 是可分发的最小单位，扩展名 `.lmpk`（UTF-8 编码 ZIP）。

```
my-package.lmpk
├── package.json           # 包描述文件
├── modules/               # 模块配置
│   └── G2p-Cmn/
│       └── config.json
└── assets/                # 资源文件（模型、字典等）
```

### 4.1 package.json

```json
{
  "packageId": "cmn-official",
  "version": "1.0.1",
  "vendor": "OpenVPI",
  "copyright": "Copyright (C) OpenVPI",

  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-cmn",
        "class": "g2p.template.MandarinG2pInference",
        "configuration": "modules/G2p-Cmn/config.json"
      }
    ]
  }
}
```

**必选**：`packageId`（禁止 `/\[]:;'"` 字符）  
**可选**：`version`, `vendor`, `copyright`, `description`, `url`, `modules`

- `modules`：按类别（`g2p` / `driver` / `dict`）声明模块数组
- 每个模块包含 `moduleId`、`class`（插件 key 匹配）、`configuration`（指向 config.json 路径）、`dependencies`（可选）

声明文件中的相对路径基于该文件所在目录。

---

## 5. 错误处理

### 5.1 Error

```cpp
class Error {
public:
    enum Type {
        Success = 0, ConfigError, FileSystemError,
        DependencyError, RuntimeError, NotImplementedError,
        InitializationError, ValidationError, NullPointerError,
        IndexError, TimeoutError
    };

    struct Context {
        std::string file;
        int line;
        std::string function;
        std::string extra;
    };

    Error(int type, std::string msg);
    Error(int type, std::string msg, std::string suggestion);

    int type() const;
    bool ok() const;
    const std::string &message() const;
    const std::string &suggestion() const;
    const Context &context() const;

    Error &withContext(const std::string &file, int line, const std::string &function);
    Error &withExtra(const std::string &extra);
    std::string fullMessage() const;
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
auto value = result.take();
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

// 数组字段
auto regexes = cfg.getStringArray("regexes");

// 字段存在性检查
if (cfg.has("pattern")) { ... }
```

### 6.2 配置加载

- Task 基类提供 `initializeConfig()` 和 `loadConfig()` 方法
- 插件在 `initialize()` 中调用 `initializeConfig()` 完成配置加载
- 配置来源：模块的 `config.json`（由 `ModuleSpec::manifestConfiguration()` 提供）

---

## 7. 日志

```cpp
#include <LangCore/Support/Logging.h>

LOG_INFO("Task initialized: {}", taskId);
LOG_ERROR("Config missing key: {}", key);
```

级别：Trace, Debug, Info, Success, Warning, Critical, Fatal。

日志分类（`ManagerLogger.h`）：`MgrLog`, `PluginLog`, `DependencyLog`, `ConfigLog`。

---

## 8. 初始化流程

```
Manager::initialize()
  → PackageManager::loadPackagesInOrder()
    → 扫描包目录，解析 package.json
    → 收集 ModuleMetadata（含 g2p/driver/dict 模块）
    → 构建依赖图，检查依赖完整性
    → 按拓扑序加载插件：PluginFactory::loadPlugin() → Plugin::createTask() → Task::initialize()
```

运行时调用：

```
Manager::convert(input) → 分发到对应 G2p Task → vector<G2pRes>
```

> 文本分割（split）和语言标注（tag）由前端负责，不再由 Manager 提供。测试代码中通过 `TestUtils::split()` / `TestUtils::tag()` 实现全流程验证。

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

### 10.1 测试中的 Splitter 与 Tagger

Splitter 和 Tagger 不再作为核心插件，而是以简化的本地实现形式存在于测试代码中（`tests/tst_langCore/`）。

**配置文件**位于 `tests/tst_langCore/configs/` 下，按 `splitter/` 和 `tagger/` 两个目录组织，每个语种一个 JSON 文件：

**Splitter 配置格式**（如 `configs/splitter/cmn.json`）：

```json
{
  "regexes": ["([\\p{Han}])"]
}
```

**Tagger 配置格式**（如 `configs/tagger/cmn.json`）：

```json
{
  "language": "cmn",
  "tagger": [
    { "type": "dict", "value": ["ds-zh-pinyin-lite.txt"], "tag": "pinyin" },
    { "type": "regex", "value": ["([\\p{Han}])"], "tag": "hanzi" }
  ]
}
```

- `type`：匹配规则类型，支持 `"regex"`（RE2 全匹配）、`"array"`（集合精确匹配）、`"dict"`（从制表符分隔文件加载词表）
- `tag`：匹配后赋予的标签
- `discard`：（可选，默认 `false`）标记为 `true` 的段落在最终结果中可被移除
- `value` 中的 dict 文件名会在 `res/G2pPackages/` 下递归查找

### 10.2 测试覆盖

全流程集成测试覆盖以下环节：

| 环节 | 测试内容 |
|------|---------|
| 加载 | 包发现、package.json 解析、插件动态加载 |
| 依赖解析 | Level/Version 校验、循环依赖检测、拓扑排序 |
| Splitter | 多语言混合文本切分、边界情况（空串、特殊字符）（test-local） |
| Tagger | 多语言标注、优先级覆盖、discard 过滤（test-local） |
| G2p | 各语言 G2p 转换正确性、批量处理 |
| 配置 | getConfig 配置读取 |
| 错误处理 | 缺失依赖、无效配置、不兼容 Level 的错误报告 |

---

## 11. 目录结构

```
core/
  include/LangCore/        公共头文件
    Base/                   NamedObject, ObjectPool, LangCommon, AlignedAllocator
    Support/                Error, Expected, ConfigAccessor, Logging, DisplayText, JSON, PhonemeDict, Tensor
    Core/                   Plugin, PluginFactory, PackageManager, Manager, ManagerLogger
    Task/                   Task, SessionTask, TaskPlugin, TaskFactory, VersionedTaskManager,
                            VersionedTaskImplBase, G2pTask, DictTask
    Module/                 Module (ModuleSpec, ModuleCategory), ModuleCategories,
                            Dependency/ (DependencyGraph, DependencyResolver, LevelCompatibilityChecker, VersionUtils)
    Package/                Package
  lib/                      实现

plugins/
  G2ps/
    MandarinG2p/            普通话 G2p（cpp-pinyin）
    CantoneseG2p/           粤语 G2p（cpp-kana）
    LstmG2p/                LSTM 模型 G2p（ONNX）
    ChainG2p/               责任链 G2p 框架
  Dicts/
    DsDict/                 字典查询
  Drivers/
    OnnxDriver/             ONNX Runtime 推理驱动
  Utils/                    辅助工具（InferUtil, OnnxUtil, Common）

res/G2pPackages/            语言包资源（含各语言的 g2p 模块配置）

tests/
  tst_langCore/
    configs/
      splitter/             Splitter 配置（每语种一个 JSON）
      tagger/               Tagger 配置（每语种一个 JSON）
    TextSplitter.h/.cpp     test-local Splitter 实现（RE2）
    TextTagger.h/.cpp       test-local Tagger 实现（RE2 + dict）
    main.cpp                全流程集成测试
```

---

## 12. 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 插件类 | `[Name]Plugin` | `MandarinG2pPlugin` |
| 任务类 | `[Name]Task` | `MandarinG2pTask` |
| 命名空间 | `LangPlugins::[Name]` | `LangPlugins::MandarinG2p` |
| 插件 key | `category.plugin-name` | `g2p.template.MandarinG2pInference` |
| 插件导出 | `LANGCORE_EXPORT_PLUGIN(Class)` | `LANGCORE_EXPORT_PLUGIN(MandarinG2pPlugin)` |
| 简化宏导出 | `LANGCORE_DEFINE_TASK_PLUGIN(...)` | `LANGCORE_DEFINE_TASK_PLUGIN(Plugin, Task, Key, Level)` |
| 模块类别宏 | `LANGCORE_DECLARE_MODULE_CATEGORY(Name, Key)` | `LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")` |

---

## 13. 依赖项

- **构建**：CMake 3.19+, C++17, qmsetup, vcpkg
- **运行时**：ONNX Runtime, cpp-pinyin, cpp-kana, RE2
- **测试**：RE2（splitter/tagger 本地实现）

---

**文档版本**: 2.4  
**最后更新**: 2026-04-21
