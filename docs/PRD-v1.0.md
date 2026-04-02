# Language Manager 产品需求文档

## 文档说明

本文档定义 Language Manager 的核心设计规范和开发指南。

**核心目标**：实现长期免维护的第三方插件加载系统，遵循"Write Once, Run Forever"的设计理念。

**版本**：1.0
**日期**：2026-04-02

---

## 目录

1. [产品概述](#1-产品概述)
2. [核心概念](#2-核心概念)
3. [目录结构设计原则](#3-目录结构设计原则)
4. [插件系统](#4-插件系统)
5. [兼容性设计](#5-兼容性设计)
6. [插件开发规范](#6-插件开发规范)
7. [Task 开发规范](#7-task-开发规范)
8. [错误处理规范](#8-错误处理规范)

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

---

## 2. 核心概念

### 2.1 Level 和 Version

**Level（API 级别）**：
- 定义：Core API 的结构版本，是 API 兼容性的唯一标准
- 格式：整数（1, 2, 3, ...）
- 规则：Level 变更表示 Core 结构体的不兼容变更

**Version（版本号）**：
- 定义：插件的 Bug 修复版本
- 格式：MAJOR.MINOR.PATCH
- 作用：不参与 API 兼容性检查，仅用于 Bug 修复记录和依赖声明

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

**SessionTask**：AI 模型驱动的特殊任务
- `open(path, args)`：打开会话
- `close()`：关闭会话
- `isOpen()`：检查会话状态
- `id()`：获取会话 ID

---

## 3. 目录结构设计原则

### 3.1 设计原则概述

Language Manager 的核心库（LangCore）采用清晰的目录结构设计，遵循以下基本原则：

**1. 声明与实现分离**
- `core/include/LangCore/`：仅包含公共头文件（声明），对外暴露的接口
- `core/lib/`：包含所有实现文件（.cpp），与 include 目录保持镜像结构

**2. 声明稳定性原则**
- `core/include/LangCore/` 目录下的头文件应该是长期不变的稳定接口
- 公共头文件的变更需要谨慎评估，遵循 Level 兼容性规则
- 内部实现细节不暴露在公共头文件中

**3. 职责单一原则**
- 每个子目录负责特定的功能领域
- 避免跨目录的强依赖关系
- 保持清晰的模块边界

**4. 依赖方向原则**
- 基础层（Base）不依赖任何其他模块
- 支持层（Support）只依赖基础层
- 核心层（Core）可以依赖基础层和支持层
- 模块层（Module）和任务层（Task）可以依赖所有下层模块

### 3.2 目录结构说明

**Base - 基础层**
```
core/include/LangCore/Base/
├── AlignedAllocator.h    # 内存对齐分配器
├── LangCommon.h          # 通用定义和宏
├── NamedObject.h         # 命名对象基类
└── ObjectPool.h          # 对象池
```
- **职责**：提供项目最基础的通用工具和基础设施
- **特点**：不依赖任何其他 LangCore 模块，可独立使用
- **稳定性**：最高级别，变更需要严格审查

**Support - 支持层**
```
core/include/LangCore/Support/
├── Error.h               # 错误处理类型
├── Expected.h            # 期望值类型（错误处理包装）
├── JSON.h                # JSON 工具类
├── Logging.h             # 日志系统
├── DisplayText.h         # 文本显示支持
├── PhonemeDict.h         # 音素字典
├── Tensor.h              # 张量处理
└── ConfigAccessor.h      # 配置访问器
```
- **职责**：提供通用工具类和辅助功能
- **特点**：只依赖 Base 层，为上层提供基础服务
- **稳定性**：高，但允许在保证兼容性前提下扩展功能
- **ConfigAccessor**：提供简洁的类型安全的配置访问接口，简化插件配置获取

**Core - 核心管理层**
```
core/include/LangCore/Core/
├── Manager.h             # 主管理器
├── ManagerLogger.h       # 管理器日志
├── PackageManager.h      # 包管理器
├── Plugin.h              # 插件接口
└── PluginFactory.h       # 插件工厂
```
- **职责**：提供系统核心管理功能，协调各模块协作
- **特点**：系统的控制中心，实现高层业务逻辑
- **稳定性**：中等，功能演进需要遵循 Level 兼容性规则

**Module - 模块层**
```
core/include/LangCore/Module/
├── Module.h              # 模块基类
├── DriverModule.h        # 驱动模块
├── G2pModule.h           # G2p 模块
├── SplitterModule.h      # 分割器模块
├── TaggerModule.h        # 标记器模块
└── Dependency/           # 依赖解析和兼容性管理子系统
    ├── DependencyGraph.h         # 依赖图
    ├── DependencyResolver.h      # 依赖解析器
    ├── VersionUtils.h            # 版本工具
    └── LevelCompatibilityChecker.h  # Level 兼容性检查器
```
- **职责**：定义各种模块类型和依赖管理
- **特点**：实现模块化的功能组织
- **稳定性**：模块接口相对稳定，内部实现可演进

**Package - 包管理层**
```
core/include/LangCore/Package/
└── Package.h             # 包定义
```
- **职责**：管理插件包的加载和解析
- **特点**：处理包格式的读取和验证
- **稳定性**：包格式需要保持向后兼容

**Task - 任务层**
```
core/include/LangCore/Task/
├── Task.h                # 任务基类
├── TaskFactory.h         # 任务工厂
├── TaskPlugin.h          # 任务插件接口
├── SessionTask.h         # 会话任务（AI 模型驱动）
├── G2pTask.h             # G2p 任务
├── SplitterTask.h        # 分割器任务
└── TaggerTask.h          # 标记器任务
```
- **职责**：定义任务执行的抽象接口和具体实现
- **特点**：插件系统的主要接口点
- **稳定性**：任务接口需要保持稳定，支持长期运行的插件

### 3.3 特殊模块位置说明

**Dependency 子系统**
- **当前位置**：`core/include/LangCore/Module/Dependency/`
- **职责**：提供依赖解析、版本管理、Level 兼容性检查和初始化顺序计算
- **评估**：位置合理，作为 Module 系统的子系统，专门负责依赖和兼容性管理
- **建议**：保持当前位置，它是模块系统的重要组成部分

**Dependency 子系统组件**

**DependencyGraph、DependencyResolver、VersionUtils**
- **职责**：处理依赖关系解析、版本范围匹配和初始化顺序计算
- **特点**：解决"版本维度"的兼容性问题

**LevelCompatibilityChecker**
- **当前位置**：`core/include/LangCore/Module/Dependency/LevelCompatibilityChecker.h`
- **职责**：提供 Level 兼容性检查功能，验证插件和依赖的 API 兼容性
- **特点**：解决"API 维度"的兼容性问题
- **与 Dependency 的关系**：
  1. 两者都解决"兼容性"问题，但关注不同维度（版本 vs API）
  2. 在实际使用中经常协同工作，共同保证插件系统的稳定运行
  3. 集中管理更符合"高内聚、低耦合"的设计原则

### 3.4 目录组织最佳实践

**1. 新增头文件放置原则**
- 如果是通用工具类 → 放入 `Support/`
- 如果是新的模块类型 → 放入 `Module/`
- 如果是新的任务类型 → 放入 `Task/`
- 如果是核心管理功能 → 放入 `Core/`
- 如果是基础工具 → 放入 `Base/`

**2. 依赖关系管理**
- 优先使用下层模块，避免跨层依赖
- 如果需要依赖上层模块，考虑是否应该重构
- 使用前向声明减少不必要的头文件包含

**3. 命名约定**
- 头文件名使用大驼峰命名（如 `LevelCompatibilityChecker.h`）
- 类名与文件名保持一致
- 避免使用缩写，使用完整的描述性名称

**4. 文档和注释**
- 公共头文件必须包含详细的文档注释
- 说明类的职责、使用方法和注意事项
- 标注稳定性级别（稳定/演进中/实验性）

### 3.5 未来扩展指南

**当需要添加新功能时**：
1. 首先评估功能的性质和职责
2. 根据功能类型选择合适的目录
3. 考虑是否需要新建子目录
4. 确保不违反依赖方向原则
5. 更新相关的文档和注释

**示例场景**：
- 添加新的通用数据结构 → `Support/`
- 添加新的模块类型 → `Module/`
- 添加新的管理功能 → `Core/`
- 添加新的任务类型 → `Task/`

---

## 4. 插件系统

### 4.1 插件接口

**TaskPlugin**：任务工厂插件

```cpp
class TaskPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Task"; }

    virtual int apiLevel() const = 0;
    virtual const char *key() const = 0;
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

**DriverPlugin**：驱动工厂插件

```cpp
class DriverPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Driver"; }

    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

**SessionFactory**：会话工厂接口（由 DriverPlugin 创建）

```cpp
class SessionFactory : public NamedObject {
public:
    virtual std::string arch() const = 0;
    virtual std::string backend() const = 0;
    virtual Expected<void> initialize(const NO<TaskInitArgs> &args) = 0;
    virtual NO<SessionTask> createSession() = 0;
};
```

### 4.2 插件注册

使用 `LANGCORE_EXPORT_PLUGIN` 宏导出插件：

```cpp
class MyPlugin final : public TaskPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "category.plugin-name"; }
    Expected<NO<Task>> createTask(const ModuleSpec *spec) override {
        return NO<MyTask>::create(spec);
    }
};

LANGCORE_EXPORT_PLUGIN(MyPlugin)
```

### 4.3 包格式

```
xxx.lmpk
  - package.json
  - modules/
  - assets/
```

**package.json 示例**：

```json
{
  "packageId": "cmn-official",
  "version": "2.0.0",
  "vendor": "Language Manager Team",
  "description": "Mandarin Chinese official package",
  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-lstm-cmn",
        "class": "g2p.lstm.LstmG2pTask",
        "configuration": "modules/lstm-g2p/config.json",
        "pluginType": "CorePlugin",
        "requiredLevel": 2,
        "dependencies": [
          {
            "packageId": "cmn-official",
            "moduleId": "splitter-cmn",
            "level": 1,
            "version": ">=1.0.0"
          }
        ]
      }
    ]
  }
}
```

### 4.4 依赖声明

支持多依赖声明：

```json
{
  "dependencies": [
    {
      "packageId": "some-package",
      "moduleId": "some-module",
      "level": 1,
      "version": ">=1.0.0"
    },
    {
      "packageId": "another-package",
      "moduleId": "another-module",
      "level": 1,
      "version": ">=1.0.0"
    }
  ]
}
```

---

## 5. 兼容性设计

### 5.1 Level 兼容性规则

**兼容条件**：

```
插件 Level = P
Manager Level = M
Manager 最小 Level = minL
Manager 最大 Level = maxL

兼容条件: minL <= P <= max(M, maxL)
```

**说明**：
- 如果 `maximumLevel = 0`，则上限为 `currentLevel`
- 如果 `maximumLevel > 0`，则上限为 `maximumLevel`

**兼容性规则表**：

| Manager Level | Plugin Level | 兼容性 | 说明 |
|---------------|--------------|--------|------|
| 2 | 2 | ✅ 兼容 | 同级别 |
| 2 | 1 | ✅ 兼容 | 向下兼容一代 |
| 2 | 0 | ❌ 不兼容 | 低于 Manager 超过 1 代 |
| 2 | 3 | ❌ 不兼容 | 高于 Manager |

### 5.2 LevelCompatibilityChecker

```cpp
class LevelCompatibilityChecker {
public:
    struct LevelConfig {
        int currentLevel;   /// 系统当前 Level
        int minimumLevel;   /// 系统最小支持 Level
        int maximumLevel;   /// 系统最大支持 Level（0 表示无限制，使用 currentLevel）

        LevelConfig(int current = 1, int minimum = 1, int maximum = 1)
            : currentLevel(current), minimumLevel(minimum), maximumLevel(maximum) {}

        /// 获取有效的最大 Level（考虑 maximumLevel=0 的情况）
        int getEffectiveMaximumLevel() const {
            return maximumLevel > 0 ? maximumLevel : currentLevel;
        }
    };

    struct ValidationResult {
        bool isCompatible;  /// 是否兼容
        int pluginLevel;    /// 插件 Level
        int systemMinimum;  /// 系统最小 Level
        int systemMaximum;  /// 系统最大 Level
        std::string message;    /// 详细消息
        std::string suggestion; /// 建议

        /// 检查是否在支持范围内
        bool isInSupportedRange() const;
    };

    /// 检查核心插件的 Level 兼容性
    static ValidationResult checkCorePlugin(int pluginLevel, const LevelConfig &config);

    /// 检查依赖插件的 Level 兼容性
    static ValidationResult checkDependencyPlugin(int pluginLevel, const LevelConfig &config);

    /// 批量检查所有插件和依赖
    static std::vector<ValidationResult>
    checkAll(const std::vector<std::pair<std::string, int>> &pluginLevels,
             const std::vector<std::pair<std::string, int>> &dependencyLevels, const LevelConfig &config);

    /// 生成验证报告
    static std::string generateReport(const std::vector<ValidationResult> &results);
};
```

### 5.3 依赖解析

**DependencyResolver**：解析模块依赖关系，计算初始化顺序

**DependencyGraph**：构建依赖关系图，检测循环依赖，计算初始化顺序

---

## 6. 插件开发规范

### 6.1 插件类型判断规则

**核心原则**：任何使用 Core 声明的结构体（如 TaskInput、TaskResult、Expected 等）的插件都是核心插件，必须进行 Level 检查。

**自动检测规则**：
1. 使用 Core 结构体 → CorePlugin
2. 继承自 Task → CorePlugin
3. 继承自 SessionTask → CorePlugin
4. 其他 → UtilityPlugin

### 6.2 TaskPlugin 开发规范

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

### 6.3 DriverPlugin 开发规范

**基本结构**：

```cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MySessionFactory.h"

namespace LangPlugins::MyDriver
{
    class MySessionFactoryPlugin final : public LangCore::DriverPlugin {
    public:
        MySessionFactoryPlugin() = default;

        LangCore::Expected<LangCore::NO<LangCore::SessionFactory>> create() override {
            return LangCore::NO<MySessionFactory>::create();
        }
    };
}

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyDriver::MySessionFactoryPlugin)
```

---

## 7. Task 开发规范

### 7.1 Task 基本结构

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

**使用 ConfigAccessor 进行配置管理**：

ConfigAccessor 提供简洁的配置访问接口，推荐用于插件开发：

```cpp
LangCore::Expected<void> initialize() override {
    auto cfg = LangCore::config(spec());

    // 必需字段 - 使用 Expected<T> 返回
    auto regexes = cfg.getStringArray("regexes");
    if (!regexes) {
        return regexes.takeError();
    }

    auto dictPath = cfg.getPath("dictPath");
    if (!dictPath) {
        return dictPath.takeError();
    }

    // 可选字段 - 带默认值
    auto enable = cfg.getBool("enable", false);
    auto threshold = cfg.getDouble("threshold", 0.5);
    
    return {};
}
```

**ConfigAccessor 主要方法**：
- **必需字段**：`getString()`, `getInt()`, `getDouble()`, `getBool()`, `getPath()`, `getStringArray()`
- **可选字段**：带默认值参数的重载版本
- **辅助方法**：`has()`, `raw()`, `basePath()`

### 7.2 Task 配置接口

**核心原则**：
- Task 配置接口使用 JSON 字符串格式
- 每个 Task 内部自行解析 JSON 配置
- 支持动态更新配置（运行时修改）

**配置接口**：

```cpp
class Task {
public:
    // 获取完整配置（JSON 字符串）
    virtual std::string getConfig() const;

    // 设置完整配置（JSON 字符串）
    virtual Expected<void> setConfig(const std::string &config);
};
```

**使用示例**：

```cpp
// 设置配置
std::string config = R"({
    "enabled": true,
    "param1": 100,
    "param2": "value"
})";
task->setConfig(config);

// 获取配置
std::string currentConfig = task->getConfig();
```

**注意事项**：
- 配置格式必须是合法的 JSON
- Task 内部使用 JSON 库解析配置（推荐 nlohmann_json）
- 配置验证由 Task 内部负责
- `setConfig()` 应该完全替换现有配置

### 7.3 自定义 Input 和 Result

**定义自定义输入**：

```cpp
class MyInput : public LangCore::TaskInput {
public:
    std::string data;
    int param1;
};
```

**定义自定义结果**：

```cpp
class MyResult : public LangCore::TaskResult {
public:
    std::string output;
    int status;
};
```

**使用自定义类型**：

```cpp
LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
    const LangCore::NO<LangCore::TaskInput> &input
) override {
    const auto myInput = input.as<MyInput>();
    if (!myInput)
        return LangCore::Error(LangCore::Error::InvalidArgument, "Invalid input type");

    auto myResult = LangCore::NO<MyResult>::create();
    myResult->output = "processed: " + myInput->data;
    return myResult;
}
```

### 7.4 SessionTask 开发规范

**基本结构**：

```cpp
class MySessionTask : public LangCore::SessionTask {
public:
    explicit MySessionTask(const LangCore::ModuleSpec *spec) : SessionTask(spec) {}

    int apiLevel() const override { return 1; }

    LangCore::Expected<void> initialize() override {
        return {};
    }

    LangCore::Expected<void> open(
        const std::filesystem::path &path,
        const LangCore::NO<LangCore::TaskInitArgs> &args
    ) override {
        m_open = true;
        return {};
    }

    LangCore::Expected<void> close() override {
        m_open = false;
        return {};
    }

    bool isOpen() const override {
        return m_open;
    }

    int64_t id() const override {
        return m_sessionId;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
        const LangCore::NO<LangCore::TaskInput> &input
    ) override {
        if (!isOpen())
            return LangCore::Error(LangCore::Error::RuntimeError, "Session not open");

        auto result = LangCore::NO<LangCore::TaskResult>::create();
        return result;
    }

private:
    bool m_open = false;
    int64_t m_sessionId = 0;
};
```

---

## 8. 错误处理规范

### 8.1 基本原则

**简洁可靠**：
- 遇到错误时直接返回 `Expected<T>` 错误
- 不设计重试机制
- 不设计回滚机制
- 错误信息应该清晰、具体
- 使用 Logger 记录关键操作和错误信息

### 8.2 错误返回

**使用 Expected 返回错误**：

```cpp
LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
    const LangCore::NO<LangCore::TaskInput> &input
) override {
    if (!input)
        return LangCore::Error(LangCore::Error::InvalidArgument, "Input is null");

    const auto myInput = input.as<MyInput>();
    if (!myInput)
        return LangCore::Error(LangCore::Error::InvalidArgument, "Invalid input type");

    if (myInput->param1 < 0)
        return LangCore::Error(LangCore::Error::InvalidArgument, "param1 must be >= 0");

    auto result = LangCore::NO<MyResult>::create();
    return result;
}
```

### 8.3 错误类型

**统一错误类型**：

```cpp
enum Type {
    Success = 0,
    ConfigError,         // 配置错误（JSON格式错误、参数错误等）
    FileSystemError,     // 文件系统错误（文件未找到、无法打开等）
    DependencyError,     // 依赖错误（循环依赖、依赖未找到等）
    RuntimeError,        // 运行时错误（会话错误、任务错误等）
    NotImplementedError, // 未实现错误（功能不支持等）
    InitializationError  // 初始化错误（未初始化、初始化失败等）
};
```

### 8.4 日志记录

使用 Logger 记录关键操作和错误信息：

```cpp
#include <LangCore/Support/Logging.h>

// 记录信息
Logger::info("Processing item: %s", item.c_str());

// 记录警告
Logger::warning("Item not found: %s", item.c_str());

// 记录错误
Logger::error("Failed to process item: %s", error.c_str());
```

---

## 9. 核心数据结构

### 9.1 G2pRes 结构

**实际实现**：

```cpp
struct G2pRes {
    std::string lyric;
    std::string g2pId;
    std::string pronunciation = lyric;
    std::vector<std::string> candidates = {pronunciation};
    std::string mode = "copy";

    explicit G2pRes(std::string lyric, std::string g2pId, std::string pronunciation = "",
                    std::vector<std::string> candidates = {}, std::string mode = "copy") :
        lyric(std::move(lyric)), g2pId(std::move(g2pId)), pronunciation(std::move(pronunciation)),
        candidates(std::move(candidates)), mode(std::move(mode)) {}
};
```

**设计说明**：
- 已移除 `error` 和 `errorType` 字段（简化设计）
- 转换成功：`pronunciation` 为转换后的发音
- 转换失败：`pronunciation` 使用默认值（原词）
- 错误信息通过 API 返回的 `Expected<T>` 处理

### 9.2 TaggerRes 结构

```cpp
struct TaggerRes {
    std::string lyric;
    std::string language = "unknown";
    std::string tag = "unknown";
    bool discard = false;

    explicit TaggerRes(std::string lyric) : lyric(std::move(lyric)) {}
    explicit TaggerRes(std::string lyric, std::string language, std::string tag) :
        lyric(std::move(lyric)), language(std::move(language)), tag(std::move(tag)) {}
};
```

### 9.3 G2pInput 结构

```cpp
struct G2pInput {
    std::string lyric;
    std::string g2pId;
    G2pInput(std::string lyric, std::string g2pId) : lyric(std::move(lyric)), g2pId(std::move(g2pId)) {}
};
```

---

## 10. 最佳实践

### 10.1 错误处理

1. **遇到错误时直接返回 `Expected<T>` 错误**
2. **不设计重试机制**
3. **不设计回滚机制**
4. **错误信息应该清晰、具体**
5. **使用 Logger 记录关键操作和错误信息**

### 10.2 配置管理

1. **使用 JSON 格式定义配置**
2. **推荐使用 ConfigAccessor**：`auto cfg = LangCore::config(spec())`
3. **必需字段使用 `Expected<T>` 返回类型的方法**（如 `getString()`）
4. **可选字段使用带默认值的方法**（如 `getString(key, defaultValue)`）
5. **支持配置热更新（通过 `setConfig`）**
6. **配置验证由 Task 内部负责**

### 10.3 插件开发

1. **插件加载后常驻内存，无需复杂生命周期管理**
2. **避免不必要的抽象层**
3. **优先保证代码简洁性**
4. **正确实现 `apiLevel()` 方法**
5. **使用兼容性检查器验证插件兼容性**

### 10.4 日志记录

1. **使用现有的日志系统**
2. **记录关键操作和错误信息**
3. **日志级别**：Trace、Debug、Info、Success、Warning、Critical、Fatal

---

## 11. 相关文档

- **构建指南**：`docs/Build-Guide.md` - 详细的构建配置和步骤
- **API 使用指南**：`docs/API-Usage-Guide.md` - Level 兼容性管理
- **任务配置 API**：`docs/Task-Config-API.md` - Task 配置接口文档
- **数据格式规范**：`docs/LangMgr-Spec-1.0.md` - 数据格式与推理接口规范

---

**文档结束**