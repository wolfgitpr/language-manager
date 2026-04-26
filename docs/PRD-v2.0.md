# Language Manager 产品需求文档 v2.0

**版本**：3.3  
**日期**：2026-04-26  
**核心目标**：C++17 插件化 G2p（Grapheme-to-Phoneme）框架，遵循"Write Once, Run Forever"设计理念。

---

## 1. 产品概述

Language Manager 是一个模块化语言处理框架。核心功能（语音转换、推理驱动、字典查询）均通过插件提供，核心库提供统一的插件管理、依赖解析和任务调度机制。

> **关于文本分割与语言标注**：Splitter（文本分割）和 Tagger（语言标注）已移至前端实现，不再属于核心框架。当前仓库中仅在测试代码（`tests/tst_langCore/`）中保留了简化的本地实现，供全流程集成测试使用。

### 1.1 设计原则

| 原则 | 含义 |
|------|------|
| 简洁可靠 | 遇错直接返回，不设计重试或回滚 |
| 接口稳定 | Level 锚定 API 结构兼容性；公共头文件即契约 |
| 长期免维护 | 插件加载后常驻内存，无复杂生命周期管理 |
| 可接受的不兼容 | Level 超出兼容范围的插件直接拒绝加载并输出清晰的错误日志，不做降级适配 |
| 异常边界隔离 | 应用层逻辑使用 `Expected<T>` 传播错误；try-catch 仅用于第三方库边界（详见 §5.4） |

### 1.2 支持语言

普通话（cmn）、粤语（yue）、日语（jpn）、英语（eng）、数字（num）、标点（punc）、未知（unknown）

---

## 2. 核心概念

### 2.1 Level 与 Version

| 概念 | 含义 | 格式 | 校验场景 |
|------|------|------|----------|
| **Level** | Core API 结构版本（函数签名、结构体布局） | 整数（1, 2, 3...） | 管理器与核心插件间、插件间依赖 |
| **Version** | 插件内部实现版本 | MAJOR.MINOR.PATCH | 插件间依赖解析（版本范围过滤） |

- **Level** 决定上层 API 是否兼容——结构体字段、函数参数变更时递增 Level。
- **Version** 描述同一 Level 下的内部实现差异。依赖方可通过版本范围约束（`>=1.0`, `~2.1`, `1.0-2.0`, `*`）选择所需的实现版本。

推荐：Version 首位与 Level 一致（Level=1 → Version=1.x.x）。

**Level 兼容性规则**（核心插件与管理器）：

```
Manager Level = M, Plugin Level = P
兼容条件: M - 1 <= P <= M
```

工具插件（Driver 等）不受此规则限制，通过依赖声明中的 Level + Version 自行校验。

**依赖解析中的双重校验**：

当模块 A 依赖模块 B 时，依赖声明同时指定 `level` 和 `version`：
1. 先按 `level` 过滤候选模块（精确匹配）
2. 再按 `version` 范围过滤（支持 `>=`, `~`, `*`, 连字符范围等语法）
3. 从满足条件的候选中选取最高版本

### 2.2 插件类型

| 类型 | 基类 | 使用 Core 结构体 | Level 检查 | 示例 |
|------|------|-----------------|-----------|------|
| **核心插件** | `TaskPlugin` | 是（TaskInput/TaskResult） | 是 | G2p, Dict |
| **工具插件** | `DriverPlugin` | 否 | 否 | OnnxDriver |

判断规则：使用 Core 结构体或继承 Task 的都是核心插件。

---

## 3. 架构

### 3.1 分层结构

```
应用层        Manager (单例，高层 API：convert, task)
               ↓ 继承
管理层        PackageManager (包发现、依赖解析、模块管理)
               ↓ 继承
工厂层        PluginFactory (动态库加载、插件实例化)
               ↓ 加载
插件层        Plugin → TaskPlugin / DriverPlugin
               ↓ 创建
任务层        Task / SessionTask
               ↓ 使用
支持层        Expected<T>, Error, ConfigAccessor, Logging, JSON, Tensor
```

### 3.2 核心组件

#### Manager

单例，顶层入口。继承 `PackageManager`。

```cpp
class Manager : public PackageManager {
public:
    static Manager *instance();
    bool initialize(std::string &errMsg);
    bool initialized() const;

    Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
    Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

    std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);
};
```

#### Plugin

插件基类。通过 DLL 导出 `langCore_plugin_instance()` C 函数暴露单例。

```cpp
class Plugin {
public:
    virtual const char *iid() const = 0;   // 接口 ID
    virtual const char *key() const = 0;   // 插件 key
    virtual int apiLevel() const = 0;      // 声明的 API Level
    std::filesystem::path path() const;    // DLL 所在路径
};
```

两种派生：

```cpp
class TaskPlugin : public Plugin {
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};

class DriverPlugin : public Plugin {
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

#### Task

处理逻辑基类。

```cpp
class Task : public NamedObject {
public:
    explicit Task(const ModuleSpec *spec);

    virtual int apiLevel() const = 0;
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

    const ModuleSpec *spec() const;
    PackageManager *Mgr() const;

    // 获取依赖模块并校验 Level
    Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;

    // 配置 JSON 字符串（供前端查询）
    virtual std::string getConfig() const;

protected:
    // 配置加载（子类在 initialize() 中调用）
    Expected<void> initializeConfig();
    Expected<std::string> loadConfig() const;
};
```

**关于 `apiLevel()` 和依赖 Level 的设计**：

- `apiLevel()` 声明本 Task **提供**的 API Level。调用方通过该方法判断能否调用。
- 依赖的 Level 要求声明在 `package.json` 的 per-dependency 字段中，由 `DependencyResolver` 在加载期静态校验。
- 运行时 Task 通过 `getObject()` 获取依赖时，框架自动校验对方的 `apiLevel()` 是否满足要求。

#### SessionTask

AI 模型驱动任务，管理推理会话。

```cpp
class SessionTask : public Task {
public:
    virtual Expected<void> open(const std::filesystem::path &path, const NO<TaskInitArgs> &args) = 0;
    virtual Expected<void> close() = 0;
    virtual bool isOpen() const = 0;
    virtual int64_t id() const = 0;
};
```

#### SessionFactory

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

#### ModuleSpec

模块元数据（id、category、className、apiLevel、manifestConfiguration、configuration、path、所属 Package）。提供本地化支持（`name()`、`configurationDisplayName()`）。

#### ModuleCategory

同类模块的容器（ObjectPool），系统预定义三类：`driver`, `g2p`, `dict`。通过宏注册：

```cpp
// 头文件
LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")
// 实现文件
LANGCORE_DEFINE_MODULE_CATEGORY(G2p, "g2p")
```

#### Package

插件包（id、version、vendor、modules、dependencies）。`ScopedPackageRef` 提供 RAII 生命周期管理。

### 3.3 简化宏

```cpp
// 定义并导出 TaskPlugin（一行完成插件类定义 + C 导出）
LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, PluginKey, ApiLevel)

// 定义并导出 DriverPlugin
LANGCORE_DEFINE_DRIVER_PLUGIN(PluginClass, FactoryClass, PluginKey, ApiLevel)

// 单版本 Task 委托代码生成（构造函数 + 方法）
TASK_IMPLEMENT(TaskClass, ImplClass)

// 多版本 Task：仅生成委托方法（构造函数手动编写）
TASK_IMPLEMENT_METHODS(TaskClass)
```

### 3.4 关键数据结构

```cpp
struct G2pInput {
    std::string lyric;    // 输入文本
    std::string g2pId;    // 使用的 G2p 模块 ID
};

struct G2pRes {
    std::string lyric;                       // 输入文本
    std::string g2pId;                       // G2p 模块 ID
    std::string pronunciation;               // 发音结果（默认为 lyric）
    std::vector<std::string> candidates;     // 候选发音
    std::string mode = "copy";               // "copy" 或 "convert"
    G2pErrorType errorType = NoError;        // 领域层错误类型
};

struct TaggerRes {
    std::string lyric;                       // 输入文本
    std::string language = "unknown";        // 语言 ID
    std::string tag = "unknown";             // 标签类型
    bool discard = false;                    // 是否丢弃
};
```

> `TaggerRes` 保留在核心数据结构中，供前端和测试使用。

### 3.5 版本化任务 I/O 类型

每个模块类别定义版本化的输入/输出类型，均继承自 `TaskInput` / `TaskResult`：

```cpp
// G2p
class G2pInputV1 : public TaskInput { std::vector<std::string> g2pInput; };
class G2pResultV1 : public TaskResult { std::vector<G2pRes> g2pResult; std::string errorMessage; };

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
class SessionResult : public TaskResult { std::map<std::string, NO<ITensor>> outputs; };

// Session 初始化参数
class DriverInitArgs : public TaskInitArgs {
    bool loadFromProcess = false;
    ExecutionProvider ep = CPUExecutionProvider;
    int deviceIndex = -1;
    std::filesystem::path runtimePath;
};
class SessionOpenArgs : public TaskInitArgs { bool useCpu = false; };

enum ExecutionProvider {
    CPUExecutionProvider, CUDAExecutionProvider,
    DMLExecutionProvider, CoreMLExecutionProvider,
};
```

### 3.6 多版本任务支持

通过 `VersionedTaskManager` 持有 `VersionedTaskImplBase` 实现并委托调用：

```cpp
class V1::TaskImpl : public VersionedTaskImplBase { ... };
class V2::TaskImpl : public VersionedTaskImplBase { ... };

class MyTask : public Task {
    VersionedTaskManager _manager;
};
```

**单版本插件**使用 `TASK_IMPLEMENT(TaskClass, ImplClass)` 一行生成全部委托代码。

**多版本插件**手动编写构造函数（按 `spec->apiLevel()` 选择实现），然后用 `TASK_IMPLEMENT_METHODS(TaskClass)` 生成剩余委托：

```cpp
MyTask::MyTask(const ModuleSpec *spec) : Task(spec), _manager(spec) {
    switch (spec->apiLevel()) {
        case 2: _manager.setImpl(std::make_unique<V2::TaskImpl>(spec)); break;
        default: _manager.setImpl(std::make_unique<V1::TaskImpl>(spec)); break;
    }
}
TASK_IMPLEMENT_METHODS(MyTask)
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

### 4.2 模块 config.json

```json
{
  "$version": "1.0.0",
  "level": 1,
  "dictPath": "dict",
  "verify": [
    { "type": "array", "value": ["SP", "AP"], "mode": "copy" },
    { "type": "regex", "value": ["[\\p{Han}]"], "mode": "convert" }
  ]
}
```

`$version` 和 `level` 由 `PackageManager` 在依赖解析阶段读取。其余字段由具体插件通过 `ConfigAccessor` 自行解析。

---

## 5. 错误处理

### 5.1 双层错误模型

| 层级 | 类型 | 用途 | 使用者 |
|------|------|------|--------|
| **框架层** | `Error` (11 codes) | 插件加载、配置、依赖解析、运行时异常 | 核心框架、`Expected<T>` |
| **领域层** | `G2pErrorType` (6 codes) | G2p 转换的具体业务错误 | `G2pRes::errorType`、前端 |

两层不合并：`Error` 服务于框架通用错误传播（`Expected<T>`），`G2pErrorType` 服务于 G2p 领域结果报告，各自语义清晰。

### 5.2 Error

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
    std::string fullMessage() const;   // 格式化完整错误消息（含 context + suggestion）
};
```

### 5.3 Expected\<T\>

替代异常的错误处理包装器（tagged union of `T` or `Error`）：

```cpp
Expected<std::string> result = someOperation();
if (!result) {
    auto err = result.takeError();
    Log.langCoreWarning("Failed: %1", err.message());
    return err;  // 直接传播，不重试，不回滚
}
auto value = result.take();
```

提供 `Expected<void>` 特化，用于无返回值的操作。

### 5.4 异常边界规则

应用层逻辑**禁止**抛出异常，一律使用 `Expected<T>` 传播错误。try-catch **仅**用于第三方库边界，将外部异常转为 `Error`：

| 边界 | 来源 | 捕获类型 | 位置 |
|------|------|----------|------|
| JSON 解析 | nlohmann/json | `std::exception` | `JSON.cpp` |
| 插件描述解析 | 文件 I/O + JSON | `std::exception` | `PluginFactory.cpp` |
| 模块配置读取 | 文件 I/O + JSON | `std::exception`, `...` | `PackageManager.cpp` |
| 版本号解析 | `std::stoi` | `...` | `VersionUtils.cpp` |
| 正则验证 | `std::regex` | `std::regex_error` | `PluginValidationUtils.cpp` |
| 类型转换 | `std::any_cast` | `std::bad_any_cast` | `G2pContext.h` |
| ONNX 推理 | ONNX Runtime | `Ort::Exception` | `Session.cpp`, `SessionImage.cpp` |

**规则**：
1. 每个 catch 块必须记录日志或返回 `Error`，禁止静默吞掉异常
2. 禁止在业务逻辑中使用 try-catch 做流程控制
3. 新增第三方库集成时，在调用入口处统一捕获并转为 `Expected<T>`

### 5.5 插件不兼容处理

当插件 Level 超出兼容范围（`M-1 <= P <= M`）时：
- 直接拒绝加载，不做降级适配
- 输出清晰的错误日志，包含：插件 key、插件 Level、管理器 Level、兼容范围
- `Error::suggestion` 引导用户升级插件或框架

---

## 6. 配置管理

### 6.1 ConfigAccessor

```cpp
auto cfg = LangCore::config(spec());

// 必需字段 — 返回 Expected<T>，缺失即报错
auto path = cfg.getPath("model_path");

// 可选字段 — 提供默认值，直接返回 T
auto threshold = cfg.getDouble("threshold", 0.5);
auto enabled = cfg.getBool("enabled", true);

// 数组字段
auto regexes = cfg.getStringArray("regexes");

// 字段存在性检查
if (cfg.has("pattern")) { ... }
```

支持的类型：`getString`, `getInt`, `getDouble`, `getBool`, `getPath`（解析为基于模块目录的绝对路径）, `getStringArray`。

### 6.2 ValidationChain

支持链式验证，返回第一个失败的错误：

```cpp
ValidationChain()
    .validateIntRange(batchSize, 1, 1000, "batchSize")
    .validateStringAllowed(mode, {"standard", "fast"}, "mode")
    .validateArrayNotEmpty(regexes, "regexes")
    .execute();
```

### 6.3 配置加载流程

- Task 基类提供 `initializeConfig()` 和 `loadConfig()` 方法
- 插件在 `initialize()` 中调用 `initializeConfig()` 完成配置加载
- 配置来源：模块的 `config.json`（由 `ModuleSpec::manifestConfiguration()` 提供）
- `ConfigAccessor` 构造时接收 `ModuleSpec*`，自动获取配置 JSON 和 basePath

---

## 7. 日志

```cpp
#include <LangCore/Support/Logging.h>

// 声明日志分类
LangCore::LogCategory Log("myPlugin");

// 使用 Qt-style %1 %2 占位符
Log.langCoreInfo("Task initialized: %1", taskId);
Log.langCoreWarning("Config missing key: %1", key);

// printf-style 变体
Log.langCoreInfoF("Loaded %d entries", count);
```

**日志级别**：Trace, Debug, Success, Information, Warning, Critical, Fatal

**内置分类**（`ManagerLogger.h`）：

| 分类 | 用途 |
|------|------|
| `MgrLog` | Manager 层操作（初始化、包加载） |
| `PluginLog` | 插件扫描、DLL 加载 |
| `DependencyLog` | 依赖解析、Level/Version 校验 |
| `ConfigLog` | 配置读取、验证 |

支持全局 `LogCallback` 和 `LogCategoryFilter` 自定义日志路由和过滤。

---

## 8. 初始化流程

```
Manager::initialize()
  → PackageManager::loadPackagesInOrder()
    → 扫描包目录，解析 package.json
    → collectModuleMetadata()：收集所有模块元数据（g2p/driver/dict）
    → DependencyResolver::resolveAllDependencies()：
        VersionResolver 按 packageId/moduleId/level/version 过滤
        迭代解析（最多 2N 轮）
    → DependencyGraph::buildGraph()：
        Tarjan SCC 检测循环依赖
        拓扑排序计算初始化顺序
    → LevelCompatibilityChecker::checkCorePlugin()：
        验证核心插件 Level 在 [minimumLevel, maximumLevel] 范围内
    → 按 PackageInitializationPlan 顺序加载：
        PluginFactory::plugin() → 懒加载 DLL → Plugin 单例
        Plugin::createTask(spec) → Task
        Task::initialize()
```

运行时调用：

```
Manager::convert(input)
  → 按 g2pId 分发到对应 G2p Task
  → Task::start(G2pInputV1) → G2pResultV1
  → 聚合为 vector<G2pRes>
```

> 文本分割和语言标注由前端负责。测试代码中通过 `TestUtils::split()` / `TestUtils::tag()` 实现全流程验证。

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

Splitter 和 Tagger 不再作为核心插件，以简化的本地实现存在于 `tests/tst_langCore/`。

**配置文件**位于 `tests/tst_langCore/configs/` 下，按 `splitter/` 和 `tagger/` 两个目录组织，每个语种一个 JSON 文件。

**Splitter 配置格式**：
```json
{ "regexes": ["([\\p{Han}])"] }
```

**Tagger 配置格式**：
```json
{
  "language": "cmn",
  "tagger": [
    { "type": "dict", "value": ["ds-zh-pinyin-lite.txt"], "tag": "pinyin" },
    { "type": "regex", "value": ["([\\p{Han}])"], "tag": "hanzi" }
  ]
}
```

- `type`：匹配规则类型——`"regex"`（RE2 全匹配）、`"array"`（集合精确匹配）、`"dict"`（从制表符分隔文件加载词表）
- `tag`：匹配后赋予的标签
- `discard`：（可选，默认 `false`）标记为 `true` 的段落可被移除
- `value` 中的 dict 文件名在 `res/G2pPackages/` 下递归查找

### 10.2 测试覆盖

| 环节 | 测试内容 |
|------|---------|
| 加载 | 包发现、package.json 解析、插件动态加载 |
| 依赖解析 | Level/Version 校验、循环依赖检测、拓扑排序 |
| Splitter | 多语言混合文本切分、边界情况（空串、特殊字符）（test-local） |
| Tagger | 多语言标注、优先级覆盖、discard 过滤（test-local） |
| G2p | 各语言 G2p 转换正确性、批量处理、性能测试 |
| 配置 | getConfig 配置读取 |
| 错误处理 | 缺失依赖、无效配置、不兼容 Level 的错误报告 |

---

## 11. 目录结构

```
core/
  include/LangCore/        公共头文件
    LangCoreGlobal.h        导出宏
    Base/                   NamedObject, ObjectPool, LangCommon, AlignedAllocator
    Support/                Error, Expected, ConfigAccessor, Logging, DisplayText, JSON,
                            PhonemeDict, Tensor
    Core/                   Plugin, PluginFactory, PackageManager, Manager, ManagerLogger
    Task/                   Task, SessionTask, TaskPlugin, TaskFactory,
                            VersionedTaskManager, VersionedTaskImplBase, G2pTask, DictTask
    Module/                 Module (ModuleSpec, ModuleCategory, ModuleLocator),
                            ModuleCategories,
                            Dependency/ (DependencyGraph, DependencyResolver,
                                        LevelCompatibilityChecker, VersionUtils)
    Package/                Package, ScopedPackageRef
  lib/                      实现

plugins/
  G2ps/
    MandarinG2p/            普通话 G2p（cpp-pinyin）
    CantoneseG2p/           粤语 G2p
    LstmG2p/                LSTM 模型 G2p（ONNX, V1+V2）
    ChainG2p/               责任链 G2p 框架（见 ChainG2p-Design-Document.md）
  Dicts/
    DsDict/                 字典查询
  Drivers/
    OnnxDriver/             ONNX Runtime 推理驱动
  Utils/                    辅助工具（InferUtil, OnnxUtil, Common）

res/G2pPackages/            语言包资源
tests/tst_langCore/         全流程集成测试
```

---

## 12. 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 插件类 | `[Name]Plugin` | `MandarinG2pPlugin` |
| 任务类 | `[Name]Task` | `MandarinG2pTask` |
| 命名空间 | `LangPlugins::[Name]` | `LangPlugins::MandarinG2p` |
| 插件 key | `category.plugin-name` | `g2p.template.MandarinG2pInference` |
| 插件导出宏 | `LANGCORE_DEFINE_TASK_PLUGIN(...)` | 见 §3.3 |
| 模块类别宏 | `LANGCORE_DECLARE_MODULE_CATEGORY(Name, Key)` | `LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")` |
| 日志分类 | `LangCore::LogCategory Log("name")` | `LangCore::LogCategory Log("onnxDriver")` |

---

## 13. 依赖项

- **构建**：CMake 3.19+, C++17, qmsetup, vcpkg, stdcorelib
- **运行时**：ONNX Runtime, cpp-pinyin, cpp-kana, nlohmann-json, blake3
- **测试**：RE2（splitter/tagger 本地实现）, Qt 6（测试基础设施）

---

## 14. 设计评审记录

本节记录经代码审计发现的设计问题及其处理状态。

### 14.1 Manager::Impl 成员遮蔽 ✅ 已修复

`Manager::Impl` 重新声明了 `initialized`、`moduleInfoSet`、`moduleInfos`，遮蔽了 `PackageManager::Impl` 的同名成员。已删除 `Manager::Impl` 中的重复声明，改为使用继承的字段。

### 14.2 VersionedTaskManager 简化 ✅ 已修复

- `VersionedTaskManager` 从模板类简化为普通类，自动从 `spec->apiLevel()` 读取 Level
- `TASK_IMPLEMENT(TaskClass, ImplClass)` 简化为 2 参数，适用于单版本插件
- 新增 `TASK_IMPLEMENT_METHODS(TaskClass)` 仅生成委托方法，供多版本插件使用（手动编写含 switch 的构造函数）
- 保留 `VersionedTaskImplBase` 接口作为多版本实现契约

### 14.3 依赖图环检测简化 ✅ 已修复

删除 Tarjan SCC 算法（~70 行），改用 Kahn 拓扑排序的副产物检测环——排序完成后未被访问的节点即为环成员。`findCycles()` 公共接口保持不变。

### 14.4 G2pErrorType 精简 ✅ 已修复

22 个枚举值精简为 6 个：`NoError`、`InvalidLyric`、`ModelInferenceFailed`、`PhonemeGenerationFailed`、`DriverUnavailable`、`UnknownError`。枚举值编号保持不变以避免序列化兼容问题。

### 14.5 ChainG2p std::any 清理 ✅ 已修复

- 删除 `WordInfo::metadata`（`map<string, any>`）——唯一的写入点已有 `bool fromFallback` 字段覆盖
- 删除 `G2pContext::m_metadata`（零读零写的死代码）和所有 metadata 访问方法
- 消除了 `std::bad_any_cast` 异常风险和对应的 try-catch

### 14.6 其他 bug 修复 ✅

- `PluginFactory::plugins<T>` 的 static_assert 从错误的 `std::is_base_of_v<std::vector<Plugin>, T>` 修正为 `std::is_base_of_v<Plugin, T>`
- `VersionUtils.cpp` 的 `catch(...)` 不再静默吞掉异常，改为记录 `DependencyLog.langCoreWarning`

### 14.7 继承链改组合（待定）

`Manager` → `PackageManager` → `PluginFactory` 三层 public 继承仍然存在。改为组合关系需要大范围重构（涉及 stdcorelib pimpl 约定、所有 `__stdc_impl_t` 宏使用点），风险较高。已通过 14.1 修复了最严重的成员遮蔽问题。完全改为组合关系作为长期目标保留。

---

### 14.8 ~~LstmG2p V1 只处理首个单词~~ ✅ 设计如此，非 Bug

`LstmG2p::Internal::V1::LstmG2pTaskImpl::start()` (line 198) 仅处理 `g2pInput[0]`，这是 **V1 的设计意图**——V1 是逐词推理实现（Level 1），V2 才是批量推理实现（Level 2）。

当前英语包 `LstmG2p-Eng/config.json` 中 `"level": 2`，因此 `LstmG2pTask` 构造时选择 V2 实现。ChainG2p 的 ModelStep 通过依赖声明 `"level": 2` 确保获取到 V2 实例。V1 仅在 Level 1 配置下使用，此时 ModelStep 应以 `batchSize: 1` 调用。

**潜在风险**：若某个 package 误配 LstmG2p 为 `level: 1` 但 ChainG2p ModelStep 的 `batchSize > 1`，V1 会丢弃首词以外的输入，触发 ModelStep 的 fallback 路径。**建议**：V1 的 `start()` 应检查输入大小，若 > 1 则返回明确错误或循环处理所有词。

### 14.9 LstmG2p V2 已完成样本继续参与解码 🟢 性能优化建议

V2 的 `start()` 在每步解码时，已生成 EOS 的样本仍在 batch 中参与计算（line 330-333 将 EOS token 作为下一步输入）。这是简单且正确的批量解码实现——LSTM 模型通常能容忍 EOS-after-EOS 输入，且 `finished` 标记确保不会将后续输出记录到 `allPredictions`。

**性能影响**：仅当 batch 内序列长度差异大时浪费明显（如最短词 3 步完成，最长词 48 步，则前者有 45 步无效计算）。

**优化方向**：对已完成样本的 decoder_input 替换为 PAD token（而非 EOS），避免模型产生不确定行为。更激进的优化是动态缩小 batch，但需要 reshape 张量，增加实现复杂度。当前实现可接受。

### 14.10 LstmG2p 硬编码 g2pId 为 "eng" 🟡 Bug

V1 和 V2 的 `start()` 中所有 `G2pRes` 构造都硬编码 `g2pId = "eng"`（如 V1 line 204, 284; V2 line 377, 380）。LstmG2p 作为通用 LSTM 推理框架，理论上可被任何语言的 ONNX 模型使用，但结果中的 g2pId 总是 "eng"。

**修复建议**：使用 `m_spec->id()` 代替硬编码 "eng"。

### 14.11 MandarinG2p/CantoneseG2p 忽略 Verifier 的 mode 分类 🟡 Bug

`MandarinG2pTaskImpl::start()` 使用 `m_verifier->verify()` 对输入词进行 mode 分类（copy/convert），然后调用 `groupLyrics()` 按 mode 分组。但在 line 110-111 中，所有分组的词都传给了 `hanziToPinyin()`，包括 `mode == "copy"` 的分组。随后 line 121 根据 mode 决定 pronunciation：`mode == "convert"` 时使用 pinyin，否则使用原词。

问题在于：对 "copy" 模式的词调用 `hanziToPinyin()` 是不必要的 I/O 和计算开销。更关键的是，`hanziToPinyin` 可能改变返回的 `hanzi` 字段（如拆分连续汉字），导致 `newRes.lyric = hanzi` 与原始输入不完全一致。

**影响**：轻微——主要是性能浪费和潜在的 lyric 不一致。

### 14.12 MandarinG2p/CantoneseG2p getConfig() 每次重建 JSON 🟢 微性能问题

`MandarinG2pTaskImpl::getConfig()` 检查 `m_config.empty()`，但 `initialize()` 从未设置 `m_config`，导致每次调用都重新构造 JSON。由于 `getConfig()` 只读取 `initialize()` 后不再变化的成员（`m_dictPath`），并发调用不存在数据竞争。但重复构造 JSON 是不必要的开销。

**修复建议**：在 `initialize()` 末尾生成并缓存 `m_config`。

### 14.13 FormatStep::addSpaceBetweenPhones 行为不符直觉 🟡 设计问题

`FormatStep::addSpaceBetweenPhones()` (line 52-72) 的逻辑：在 alphanumeric 字符后遇到非空格、非 alphanumeric 字符时插入空格。但不处理「多个连续空格」或「音素间已有空格」的情况。例如输入 `"AH0 L OW1"` 不会被改变（已有空格），但 `"AH0L"` 也不会被拆分（因为 `isalnum` 对数字也返回 true，`'0'` 和 `'L'` 之间不会插入空格）。

**影响**：该函数名暗示「在音素之间加空格」，但实际行为更像是「在 alphanumeric 和非 alphanumeric 之间加空格」。需要明确文档或重命名。

### 14.14 PackageManager::checkDependencies 首个不兼容即返回 🟡 设计问题

`PackageManager::checkDependencies()` 在 Level 兼容性检查循环中（line ~383），遇到第一个不兼容模块就 `return false`。这意味着用户只能看到第一个不兼容模块的错误信息，需要反复修复、重启才能发现所有不兼容模块。

**修复建议**：收集所有不兼容模块的错误信息后再返回 false，让用户一次性看到所有问题。

### 14.15 Expected<T> 默认构造值初始化 🟡 设计问题

`Expected<T>` 的默认构造函数 (Expected.h line 41) 使用 `value_type{}` 值初始化。对于不可默认构造的类型 T，这会导致编译错误。虽然当前代码中没有使用不可默认构造类型的 Expected，但作为通用库类型，应考虑删除默认构造函数或使用 SFINAE 约束。

### 14.16 PluginFactory 的 pluginsDirty 从不清除 🟢 微性能问题

`PluginFactory::scanPlugins()` 完成后未从 `pluginsDirty` 中移除已扫描的 iid。后续每次调用 `plugin()`/`plugins()` 都会重新进入 `scanPlugins()`，虽然 `scannedPluginDirs` 缓存阻止了重复 DLL 加载，但仍有不必要的目录遍历和锁竞争。

**实际影响**：`plugin()` 仅在初始化阶段被调用（每个模块一次），运行时不调用，因此开销可忽略。

**修复建议**：在 `scanPlugins()` 末尾调用 `pluginsDirty.erase(iid)`，使代码语义更清晰。

### 14.17 Session::close 中 hash_size_map 查找可能崩溃 🟡 潜在 Bug

`Session::close()` (Session.cpp line 600) 在 `images.empty()` 时查找 `hash_size_map.find({group.size, group.hash})`。如果 `Session::open()` 走了 `out_search_hash` 路径（即 path_map 命中但 hash 未计算），则 `group.hash` 可能为空 vector，而 `hash_size_map` 中对应的 key 是通过 `it->second->hash` 引用的。由于 `out_search_hash` 路径之后的 `image_group` 是从 `path_map` 获取的已有 group，其 hash 在首次创建时已被设置，所以实际上不会出问题。但代码的控制流（goto labels）使得正确性推理很困难。

**建议**：用结构化控制流替代 goto，提高可读性和可维护性。

---

**文档版本**: 3.3  
**最后更新**: 2026-04-26
