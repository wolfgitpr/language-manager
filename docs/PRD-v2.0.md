# Language Manager 产品需求文档 v2.0

**版本**：3.2  
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
| **领域层** | `G2pErrorType` (22 codes) | G2p 转换的具体业务错误 | `G2pRes::errorType`、前端 |

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

**文档版本**: 3.2  
**最后更新**: 2026-04-26
