# Language Manager 产品需求文档

## 文档说明

本文档定义 Language Manager 的核心设计规范和开发指南。

**核心目标**：实现长期免维护的第三方插件加载系统，遵循"Write Once, Run Forever"的设计理念。

**版本**：2.0
**日期**：2026-04-03

---

## 目录

1. [产品概述](#1-产品概述)
2. [核心概念](#2-核心概念)
3. [插件系统](#3-插件系统)
4. [兼容性设计](#4-兼容性设计)
5. [插件开发规范](#5-插件开发规范)
6. [Task 开发规范](#6-task-开发规范)
7. [错误处理规范](#7-错误处理规范)
8. [核心数据结构](#8-核心数据结构)
9. [最佳实践](#9-最佳实践)
10. [相关文档](#10-相关文档)

---

## 1. 产品概述

Language Manager 是一个基于 C++ 的可扩展语言处理框架，主要用于文本到语音转换（G2p）。

**核心特性**：
- **插件化架构**：所有功能模块作为插件动态加载
- **Level 兼容性系统**：Level 是 API 兼容性的唯一标准
- **包管理系统**：支持插件包的发现、依赖解析和加载
- **长期稳定性**：插件加载后常驻内存，无需复杂生命周期管理

**设计原则**：
- **简洁可靠**：允许必要时直接报错，记录日志，不设计重试或回滚机制
- **插件分类**：核心插件（使用 Core 结构体）和工具插件（独立功能）
- **统一规则**：Level 作为 API 兼容性的唯一标准

**支持的语言**：
- 普通话（cmn）、粤语（yue）、日语（jpn）、英语（eng）
- 数字（num）、标点符号（punc）、未知（unknown）

---

## 2. 核心概念

### 2.1 Level 和 Version

**Level（API 级别）**：
- 定义：Core API 的结构版本，是 API 兼容性的**唯一标准**
- 格式：整数（1, 2, 3, ...）
- 规则：Level 变更表示 Core 结构体的不兼容变更

**Version（版本号）**：
- 定义：插件的 Bug 修复版本
- 格式：MAJOR.MINOR.PATCH
- 作用：**不参与** API 兼容性检查，仅用于 Bug 修复记录和依赖声明
- 推荐：Version 第一位与 Level 相同（例如：Level=1, Version=1.2.3）

### 2.2 插件类型

**核心插件（CorePlugin）**：
- 使用 Core 结构体（如 TaskInput、TaskResult）
- 需要 Level 检查
- 示例：Splitter、Tagger、G2p

**工具插件（UtilityPlugin）**：
- 独立功能，不使用 Core 结构体
- 不需要 Level 检查
- 示例：辅助工具、转换器

**判断规则**：任何使用 Core 声明的结构体或继承自 Task 的插件都是核心插件。

### 2.3 核心组件

**Manager**：提供高层 API，协调插件、包、任务的完整工作流程
- `task(category, id)`：获取指定任务
- `tasks(category)`：获取指定类别的所有任务
- `split(input)`：文本分割
- `tag(input)`：语言标记
- `convert(input)`：G2p 转换

**Task**：实际执行处理逻辑的单元，继承自 NamedObject
- `apiLevel()`：返回 API 级别
- `initialize()`：初始化
- `start(input)`：执行任务
- `getConfig()`：获取配置
- `setConfig(config)`：设置配置
- `getUiSchema()`：获取 UI Schema
- `resetToDefault()`：恢复默认配置
- `isUsingDefaultConfig()`：检查是否使用默认配置

**SessionTask**：AI 模型驱动的特殊任务
- `open(path, args)`：打开会话
- `close()`：关闭会话
- `isOpen()`：检查会话状态
- `id()`：获取会话 ID

**SessionFactory**：会话工厂接口
- `arch()`：返回架构类型
- `backend()`：返回后端类型
- `initialize(args)`：初始化工厂
- `createSession()`：创建会话

---

## 3. 插件系统

### 3.1 插件接口

**Plugin 基类**：
```cpp
class Plugin {
public:
    virtual const char *iid() const = 0;  // 接口标识符
    virtual const char *key() const = 0;  // 插件键名
    virtual int apiLevel() const = 0;    // API 级别
};
```

**TaskPlugin 接口**：
```cpp
class TaskPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

**DriverPlugin 接口**：
```cpp
class DriverPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

### 3.2 插件注册

使用 `LANGCORE_EXPORT_PLUGIN` 宏导出插件：

```cpp
LANGCORE_EXPORT_PLUGIN(MyPlugin)
```

### 3.3 包管理

**包格式**：ZIP 压缩包（.lmpk），包含：
- `package.json`：包的元数据和配置
- `modules/`：模块定义
- `configs/`：用户自定义配置
- `assets/`：资源文件

**包结构示例**：
```json
{
  "packageId": "cmn-official",
  "version": "1.0.1",
  "vendor": "小狼Korou",
  "modules": {
    "splitter": [{"moduleId": "splitter-cmn", ...}],
    "tagger": [{"moduleId": "tagger-cmn", ...}],
    "g2p": [{"moduleId": "g2p-cmn", ...}]
  }
}
```

---

## 4. 兼容性设计

### 4.1 Level 兼容性规则

**核心插件（Splitter/Tagger/G2p）**：

```
Manager Level = M
Plugin Level = P

兼容条件: M - 1 <= P <= M
```

| Manager Level | Plugin Level | 兼容性 | 说明 |
|---------------|--------------|--------|------|
| 2 | 2 | ✅ 兼容 | 同级别 |
| 2 | 1 | ✅ 兼容 | 向下兼容一代 |
| 2 | 0 | ❌ 不兼容 | 低于 Manager 超过 1 代 |
| 2 | 3 | ❌ 不兼容 | 高于 Manager |

**依赖插件（Driver 等）**：
- 不对 Level 进行限制
- 通过 dependency 系统自动分析

### 4.2 LevelCompatibilityChecker

```cpp
class LevelCompatibilityChecker {
public:
    struct LevelConfig {
        int currentLevel;   // 系统当前 Level
        int minimumLevel;   // 系统最小支持 Level
        int maximumLevel;   // 系统最大支持 Level（0 表示无限制）
    };

    struct ValidationResult {
        bool isCompatible;      // 是否兼容
        int pluginLevel;        // 插件 Level
        int systemMinimum;      // 系统最小 Level
        int systemMaximum;      // 系统最大 Level
        std::string message;    // 详细消息
        std::string suggestion; // 建议
    };

    // 检查核心插件兼容性
    static ValidationResult checkCorePlugin(int pluginLevel, const LevelConfig &config);

    // 检查依赖插件兼容性
    static ValidationResult checkDependencyPlugin(int pluginLevel, const LevelConfig &config);

    // 批量检查
    static std::vector<ValidationResult> checkAll(
        const std::vector<std::pair<std::string, int>> &pluginLevels,
        const std::vector<std::pair<std::string, int>> &dependencyLevels,
        const LevelConfig &config
    );

    // 生成报告
    static std::string generateReport(const std::vector<ValidationResult> &results);
};
```

### 4.3 依赖解析

**DependencyResolver**：解析模块依赖关系，计算初始化顺序
**DependencyGraph**：构建依赖关系图，检测循环依赖

---

## 5. 插件开发规范

### 5.1 插件类型判断规则

**自动检测规则**：
1. 使用 Core 结构体 → CorePlugin
2. 继承自 Task → CorePlugin
3. 继承自 SessionTask → CorePlugin
4. 其他 → UtilityPlugin

### 5.2 TaskPlugin 开发规范

**基本结构**：

```cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MyTask.h"

namespace LangPlugins::MyPlugin
{
    class MyTaskPlugin final : public LangCore::TaskPlugin {
    public:
        MyTaskPlugin() = default;

        int apiLevel() const override {
            return 1;
        }

        const char *key() const override {
            return "category.plugin-name";
        }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<MyTask>::create(spec);
        }
    };
}

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyPlugin::MyTaskPlugin)
```

**命名规范**：
- 插件类名：`[Name]Plugin`
- 命名空间：`LangPlugins::[PluginName]`
- key 值：`category.plugin-name`

**类别规范**：
- `g2p` - G2p 转换插件
- `splitter` - 文本分割插件
- `tagger` - 语言标记插件
- `driver` - AI 模型驱动插件

### 5.3 使用插件注册宏简化开发

**LANGCORE_DEFINE_TASK_PLUGIN 宏**：

```cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MyTask.h"

using namespace LangCore;
using namespace LangPlugins::MyPlugin;

LANGCORE_DEFINE_TASK_PLUGIN(
    MyTaskPlugin,           // 插件类名
    MyTask,                 // Task 类名
    "category.plugin-name", // 插件 key
    1                       // API Level
)
```

**LANGCORE_DEFINE_DRIVER_PLUGIN 宏**：

```cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MySessionFactory.h"

using namespace LangCore;
using namespace LangPlugins::MyDriver;

LANGCORE_DEFINE_DRIVER_PLUGIN(
    MySessionFactoryPlugin, // 插件类名
    MySessionFactory,       // SessionFactory 类名
    "onnx",                // 插件 key
    1                      // API Level
)
```

**优势**：
- 减少约 15 行重复代码
- 提高代码一致性
- 简化插件开发流程

**注意事项**：
1. 使用宏前必须包含 `<LangCore/Task/TaskPlugin.h>`
2. 使用 `using namespace LangCore;` 和相应的插件命名空间
3. 类名不要包含命名空间前缀
4. API Level 应该与 Task 的实现级别一致

### 5.4 DriverPlugin 开发规范

**基本结构**：

```cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MySessionFactory.h"

namespace LangPlugins::MyDriver
{
    class MySessionFactoryPlugin final : public LangCore::DriverPlugin {
    public:
        MySessionFactoryPlugin() = default;

        int apiLevel() const override {
            return 1;
        }

        const char *key() const override {
            return "driver-name";
        }

        LangCore::Expected<LangCore::NO<LangCore::SessionFactory>> create() override {
            return LangCore::NO<MySessionFactory>::create();
        }
    };
}

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyDriver::MySessionFactoryPlugin)
```

---

## 6. Task 开发规范

### 6.1 Task 基本结构

**最小实现**：

```cpp
#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>

class MyTask : public LangCore::Task {
public:
    explicit MyTask(const LangCore::ModuleSpec *spec) : LangCore::Task(spec) {}

    int apiLevel() const override {
        return 1;
    }

    LangCore::Expected<void> initialize() override {
        // 使用 ConfigAccessor 获取配置
        auto cfg = LangCore::config(spec());

        // 获取必需字段
        auto requiredField = cfg.getString("requiredField");
        if (!requiredField) {
            return requiredField.takeError();
        }

        // 获取可选字段
        auto optionalField = cfg.getBool("optionalField", false);

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
        const LangCore::NO<LangCore::TaskInput> &input
    ) override {
        auto result = LangCore::NO<LangCore::TaskResult>::create();
        return result;
    }
};
```

### 6.2 使用 ConfigAccessor 进行配置管理

ConfigAccessor 提供简洁的配置访问接口：

```cpp
auto cfg = LangCore::config(spec());

// 必需字段
auto str = cfg.getString("key");
auto num = cfg.getInt("key");
auto path = cfg.getPath("key");
auto arr = cfg.getStringArray("key");

// 可选字段
auto str = cfg.getString("key", "default");
auto num = cfg.getInt("key", 0);
auto flag = cfg.getBool("key", false);
```

### 6.3 配置 API

**获取配置**：
```cpp
std::string config = task->getConfig();
```

**设置配置**：
```cpp
std::string newConfig = R"({"enabled": true, "param": "value"})";
auto result = task->setConfig(newConfig);
if (!result) {
    // 处理错误
}
```

**恢复默认配置**：
```cpp
task->resetToDefault();
```

**检查是否使用默认配置**：
```cpp
if (!task->isUsingDefaultConfig()) {
    // 使用用户自定义配置
}
```

### 6.4 多版本 Task 实现

对于需要支持多个 Level 的 Task，使用 `VersionedTaskManager`：

```cpp
template<typename TaskType>
class VersionedTaskManager {
public:
    explicit VersionedTaskManager(const ModuleSpec *spec);

    int currentLevel() const;
    void setCurrentLevel(int level);
    const ModuleSpec *spec() const;
    VersionedTaskImplBase *impl() const;
    void setImpl(std::unique_ptr<VersionedTaskImplBase> impl);

    Expected<void> initialize();
    Expected<NO<TaskResult>> start(const NO<TaskInput> &input);
    std::string getConfig() const;
    Expected<void> setConfig(const std::string &config);
};
```

**实现模式**：
```
LstmG2pTaskImplBase（基类）
├── 共同成员变量和方法
└── 虚方法：start()
    ├── V1::LstmG2pTaskImpl（逐词处理）
    └── V2::LstmG2pTaskImpl（批量推理）
```

---

## 7. 错误处理规范

### 7.1 Error 类

```cpp
class Error {
public:
    enum Type {
        Success = 0,
        ConfigError,        // 配置错误
        FileSystemError,    // 文件系统错误
        DependencyError,    // 依赖错误
        RuntimeError,       // 运行时错误
        NotImplementedError, // 未实现错误
        InitializationError // 初始化错误
    };

    // 构造函数
    Error(const Type type);
    Error(const int type, std::string msg);
    Error(const int type, std::string msg, std::string suggestion);

    // 访问方法
    int type() const;
    bool ok() const;
    const std::string &message() const;
    const std::string &suggestion() const;
    bool hasSuggestion() const;

    static Error success();
};
```

**使用示例**：
```cpp
// 创建带建议的错误
auto error = Error(Error::ConfigError,
                   "Missing required field: regexes",
                   "Add the 'regexes' field to the configuration");

// 检查错误
if (!result) {
    auto err = result.takeError();
    std::cerr << "Error: " << err.message() << std::endl;
    if (err.hasSuggestion()) {
        std::cerr << "Suggestion: " << err.suggestion() << std::endl;
    }
}
```

### 7.2 Expected<T> 类型

Expected<T> 是一个错误处理包装器，用于替代异常：

```cpp
template<typename T>
class Expected {
public:
    Expected(T value);
    Expected(Error error);

    bool ok() const;
    T value() const;
    Error error() const;
    Error takeError();
};
```

**使用规范**：
1. **遇到错误时直接返回 `Expected<T>` 错误**
2. **不设计重试机制**
3. **不设计回滚机制**
4. **错误信息应该清晰、具体**
5. **使用 Logger 记录关键操作和错误信息**

---

## 8. 核心数据结构

### 8.1 G2pInput

```cpp
struct G2pInput {
    std::string lyric;    // 歌词/文本
    std::string g2pId;    // G2p ID
    G2pInput(std::string lyric, std::string g2pId);
};
```

### 8.2 G2pRes

```cpp
struct G2pRes {
    std::string lyric;                          // 歌词/文本
    std::string g2pId;                          // G2p ID
    std::string pronunciation = lyric;          // 发音结果
    std::vector<std::string> candidates;        // 候选发音
    std::string mode = "copy";                  // 模式: "copy" 或 "convert"
};
```

**设计说明**：
- 转换成功：`pronunciation` 为转换后的发音
- 转换失败：`pronunciation` 使用默认值（原词）
- 错误信息通过 API 返回的 `Expected<T>` 处理

### 8.3 TaggerRes

```cpp
struct TaggerRes {
    std::string lyric;              // 歌词/文本
    std::string language = "unknown"; // 语言 ID
    std::string tag = "unknown";    // 标签类型
    bool discard = false;           // 是否丢弃
};
```

### 8.4 TaskInput/TaskResult

```cpp
class TaskInput : public TaskInfoBase {
public:
    explicit TaskInput() = default;
};

class TaskResult : public TaskInfoBase {
public:
    explicit TaskResult() = default;
    Error error;
};
```

### 8.5 SplitterInputV1/SplitterResultV1

```cpp
struct SplitterInputV1 {
    std::vector<std::string> splitterInput;
};

struct SplitterResultV1 {
    std::vector<std::string> splitterResult;
};
```

---

## 9. 最佳实践

### 9.1 错误处理
1. **遇到错误时直接返回 `Expected<T>` 错误**
2. **不设计重试机制**
3. **不设计回滚机制**
4. **错误信息应该清晰、具体**
5. **使用 Logger 记录关键操作和错误信息**

### 9.2 配置管理
1. **使用 JSON 格式定义配置**
2. **推荐使用 ConfigAccessor**：`auto cfg = LangCore::config(spec())`
3. **必需字段使用 `Expected<T>` 返回类型的方法**（如 `getString()`）
4. **可选字段使用带默认值的方法**（如 `getString(key, defaultValue)`）
5. **支持配置热更新（通过 `setConfig`）**
6. **配置验证由 Task 内部负责**

### 9.3 插件开发
1. **插件加载后常驻内存，无需复杂生命周期管理**
2. **避免不必要的抽象层**
3. **优先保证代码简洁性**
4. **正确实现 `apiLevel()` 方法**
5. **使用兼容性检查器验证插件兼容性**

### 9.4 日志记录
1. **使用现有的日志系统**
2. **记录关键操作和错误信息**
3. **日志级别**：Trace、Debug、Info、Success、Warning、Critical、Fatal

---

## 10. 相关文档

- **API 使用指南**：`docs/API-Usage-Guide.md` - Level 兼容性管理
- **任务配置 API**：`docs/Task-Config-API.md` - Task 配置接口文档
- **UI 配置 Schema**：`docs/Plugin-UI-Configuration-Schema.md` - Task UI Schema 规范
- **数据格式规范**：`docs/LangMgr-Spec-1.0.md` - 数据格式与推理接口规范
- **配置持久化**：`docs/Config-Persistence-Design.md` - 配置持久化设计方案

---

**文档版本**: 2.0
**最后更新**: 2026-04-03
**更新内容**:
- 精简文档，移除冗余内容
- 保留关键信息和必要细节
- 优化文档结构，提高可读性
- 添加插件注册宏说明
- 更新最佳实践章节