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

## 9. 多版本插件代码组织方案

### 9.1 设计目标

Language Manager 需要支持同一个插件内包含多个 Level 版本的代码实现，以满足以下需求：

1. **对外接口不变**：Plugin 和 Task 的公共接口保持稳定，不随 Level 变化而改变
2. **内部按需加载**：根据 `spec()->apiLevel()` 动态选择对应版本的实现代码
3. **向下兼容**：每个插件向下兼容到 Level 1，确保旧版本插件仍能工作
4. **运行时稳定**：运行时只能更新配置，不能改变 Level 等级
5. **代码组织清晰**：通过文件和命名空间清晰地区分不同版本的实现
6. **编译时优化**：未使用的 Level 版本代码可以优化掉
7. **代码复用**：通过 core 库中的抽象基类避免重复代码

### 9.2 文件组织规范

#### 9.2.1 目录结构

```
plugins/[Category]/[PluginName]/
├── CMakeLists.txt                    # 构建配置
├── main.cpp                          # Plugin 入口（统一接口）
├── Task.h                            # Task 类定义（统一接口）
├── Task.cpp                          # Task 类实现
└── internal/                         # 内部实现目录（不对外暴露）
    ├── TaskImplBase.h                # TaskImplBase 接口定义（使用 core 抽象）
    ├── V1/                           # Level 1 实现
    │   ├── TaskImpl.h                # Level 1 Task 实现
    │   └── TaskImpl.cpp
    ├── V2/                           # Level 2 实现（可选）
    │   ├── TaskImpl.h
    │   └── TaskImpl.cpp
    └── V3/                           # Level 3 实现（可选）
        ├── TaskImpl.h
        └── TaskImpl.cpp
```

**目录命名规范**：
- `internal/`：内部实现目录，不对外暴露
- `V1/`, `V2/`, `V3/`：分别对应 Level 1、Level 2、Level 3 的实现
- 每个版本目录包含独立的头文件和实现文件
- **当前状态**：所有插件目前只有 V1 实现，V2/V3 为可选扩展

#### 9.2.2 文件命名规范

**Plugin 相关文件**：
- `main.cpp`：Plugin 入口，导出 Plugin 类

**Task 相关文件**：
- `Task.h`：Task 类定义（统一接口）
- `Task.cpp`：Task 类实现（使用 VersionedTaskManager）
- `internal/TaskImplBase.h`：TaskImplBase 接口定义（使用 core 抽象）
- `internal/V1/TaskImpl.h`：Level 1 Task 实现
- `internal/V1/TaskImpl.cpp`：Level 1 Task 实现
- `internal/V2/TaskImpl.h`：Level 2 Task 实现（可选）
- `internal/V2/TaskImpl.cpp`：Level 2 Task 实现（可选）

### 9.3 命名空间规范

#### 9.3.1 命名空间层次结构

```
LangPlugins::[PluginName]              # 插件命名空间
├── [PluginName]Task                   # Task 类
└── Internal                           # 内部实现命名空间（不对外暴露）
    ├── V1                             # Level 1 实现
    │   └── [PluginName]TaskImpl       # Level 1 Task 实现
    ├── V2                             # Level 2 实现（可选）
    │   └── [PluginName]TaskImpl       # Level 2 Task 实现
    └── V3                             # Level 3 实现（可选）
        └── [PluginName]TaskImpl       # Level 3 Task 实现
```

**命名规范**：
- 插件命名空间：`LangPlugins::[PluginName]`
- 内部实现命名空间：`LangPlugins::[PluginName]::Internal`
- 版本命名空间：`LangPlugins::[PluginName]::Internal::V1`、`V2`、`V3`
- 实现类名：`[PluginName]TaskImpl`（在不同版本命名空间中）

#### 9.3.2 命名空间使用示例

```cpp
// Task 类（对外接口）
namespace LangPlugins::RegexSplitter {
    class RegexSplitterTask : public LangCore::Task {
        // ... 使用 VersionedTaskManager
    };
}

// Level 1 实现（内部）
namespace LangPlugins::RegexSplitter::Internal::V1 {
    class RegexSplitterTaskImpl {
        // ... Level 1 具体实现
    };
}

// Level 2 实现（内部，可选）
namespace LangPlugins::RegexSplitter::Internal::V2 {
    class RegexSplitterTaskImpl {
        // ... Level 2 具体实现
    };
}
```

### 9.4 Core 抽象基类

#### 9.4.1 VersionedTaskImplBase

`LangCore::VersionedTaskImplBase` 是 core 库中定义的抽象基类，所有插件的版本实现都必须继承这个接口。

**接口定义**（位于 `core/include/LangCore/Task/VersionedTaskImplBase.h`）：

```cpp
namespace LangCore
{
    class VersionedTaskImplBase {
    public:
        virtual ~VersionedTaskImplBase() = default;

        virtual Expected<void> initialize() = 0;
        virtual Expected<NO<TaskResult>>
        start(const NO<TaskInput> &input) = 0;
        virtual std::string getConfig() const = 0;
        virtual Expected<void> setConfig(const std::string &config) = 0;
    };
}
```

**使用方式**：

```cpp
// internal/TaskImplBase.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_TASKIMPLBASE_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_TASKIMPLBASE_H

#include <LangCore/Task/VersionedTaskImplBase.h>

namespace LangPlugins::RegexSplitter::Internal
{
    /// TaskImplBase 是所有版本实现的基类接口
    /// 继承自 LangCore::VersionedTaskImplBase，使用 core 中定义的稳定接口
    using TaskImplBase = LangCore::VersionedTaskImplBase;
}

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_TASKIMPLBASE_H
```

#### 9.4.2 VersionedTaskManager

`LangCore::VersionedTaskManager` 是 core 库中提供的版本管理辅助类，用于管理版本存储和实现委托。

**类定义**（位于 `core/include/LangCore/Task/VersionedTaskManager.h`）：

```cpp
namespace LangCore
{
    template<typename TaskType>
    class VersionedTaskManager {
    public:
        struct VersionedImpl {
            const ModuleSpec *spec = nullptr;
            std::unique_ptr<VersionedTaskImplBase> impl;
            int currentLevel = 1;

            explicit VersionedImpl(const ModuleSpec *s) : spec(s) {}
        };

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

    protected:
        std::unique_ptr<VersionedImpl> _impl;
    };
}
```

**设计原则**：
- 只提供数据结构和辅助方法
- 不包含具体的版本选择逻辑
- 允许插件自定义版本选择策略

### 9.5 实现方式

#### 9.5.1 Plugin 实现

Plugin 类保持不变，负责创建 Task 实例：

```cpp
// main.cpp
#include <LangCore/Task/TaskPlugin.h>
#include "Task.h"

namespace LangPlugins::RegexSplitter
{
    LANGCORE_EXPORT_PLUGIN(RegexSplitterPlugin)
}
```

```cpp
// Plugin.cpp（如果存在）
#include "Task.h"

namespace LangPlugins::RegexSplitter
{
    int RegexSplitterPlugin::apiLevel() const {
        return 1;  // 返回当前插件支持的最高 Level
    }

    const char *RegexSplitterPlugin::key() const {
        return "splitter.regex.RegexSplitter";
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    RegexSplitterPlugin::createTask(const LangCore::ModuleSpec *spec) {
        return LangCore::NO<RegexSplitterTask>::create(spec);
    }
}
```

#### 9.5.2 Task 类实现

Task 类使用 `VersionedTaskManager` 来管理版本：

```cpp
// Task.h
#ifndef LANGPLUGINS_REGEXSPLITTER_TASK_H
#define LANGPLUGINS_REGEXSPLITTER_TASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <memory>

namespace LangPlugins::RegexSplitter
{
    class RegexSplitterTask : public LangCore::Task {
    public:
        explicit RegexSplitterTask(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTask() override;

        int apiLevel() const override;
        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<RegexSplitterTask> _manager;
    };
} // namespace LangPlugins::RegexSplitter

#endif // LANGPLUGINS_REGEXSPLITTER_TASK_H
```

```cpp
// Task.cpp
#include "RegexSplitterTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::RegexSplitter
{
    RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::RegexSplitterTaskImpl>(spec));
    }

    RegexSplitterTask::~RegexSplitterTask() = default;

    int RegexSplitterTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> RegexSplitterTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string RegexSplitterTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> RegexSplitterTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::RegexSplitter
```

#### 9.5.3 Level 1 实现

```cpp
// internal/V1/TaskImpl.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal::V1
{
    class RegexSplitterTaskImpl : public Internal::TaskImplBase {
    public:
        explicit RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        std::string m_config;
    };
} // namespace LangPlugins::RegexSplitter::Internal::V1

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_V1_TASKIMPL_H
```

```cpp
// internal/V1/TaskImpl.cpp
#include "TaskImpl.h"
#include <LangCore/Task/SplitterTask.h>
#include <LangCore/Support/Error.h>

namespace LangPlugins::RegexSplitter::Internal::V1
{
    RegexSplitterTaskImpl::RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> RegexSplitterTaskImpl::initialize() {
        auto cfg = LangCore::config(m_spec);
        auto pattern = cfg.getString("pattern");
        if (!pattern) {
            return pattern.takeError();
        }

        m_config = R"({"pattern": ")" + *pattern + R"("})";
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        const auto splitterInput = input.as<LangCore::SplitterInputV1>();
        if (!splitterInput) {
            return LangCore::Error(LangCore::Error::RuntimeError,
                                  "Invalid input type for Level 1");
        }

        auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
        result->splitterResult = splitterInput->splitterInput;

        return result;
    }

    std::string RegexSplitterTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> RegexSplitterTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }
} // namespace LangPlugins::RegexSplitter::Internal::V1
```

### 9.6 扩展到多版本

当需要添加新版本时（例如 Level 2），只需要：

1. 创建 `internal/V2/TaskImpl.h` 和 `TaskImpl.cpp`
2. 在 `Task.cpp` 中添加版本选择逻辑

```cpp
// Task.cpp
#include "RegexSplitterTask.h"
#include "internal/V1/TaskImpl.h"
#include "internal/V2/TaskImpl.h"  // 新增

namespace LangPlugins::RegexSplitter
{
    RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        if (level >= 2) {
            _manager.setImpl(std::make_unique<Internal::V2::RegexSplitterTaskImpl>(spec));
        } else {
            // Level 1 作为默认（向下兼容）
            _manager.setImpl(std::make_unique<Internal::V1::RegexSplitterTaskImpl>(spec));
        }
    }
    // ... 其他方法保持不变
}
```

### 9.7 总结

多版本插件代码组织方案的核心要点：

1. **文件组织清晰**：通过 `internal/Vx/` 目录结构清晰地区分不同版本
2. **命名空间隔离**：使用 `Internal::Vx` 命名空间隔离不同版本的实现
3. **使用 core 抽象**：所有插件使用 `VersionedTaskImplBase` 和 `VersionedTaskManager`
4. **对外接口稳定**：Plugin 和 Task 的公共接口保持不变
5. **向下兼容保证**：Level 1 作为默认实现，确保兼容性
6. **运行时稳定**：配置更新不改变 Level 等级
7. **易于扩展**：新增 Level 版本只需添加新的实现类
8. **代码复用**：通过 core 库的抽象基类避免重复代码
9. **当前状态**：所有插件目前只有 V1 实现，V2/V3 为可选扩展

这个方案满足"Write Once, Run Forever"的设计理念，确保插件系统在 API 演进的同时保持长期稳定性。
    }

    LangCore::Expected<void> RegexSplitterTaskImpl::setConfig(const std::string &config) {
        // Level 1 的配置更新逻辑
        // 配置验证：确保不包含 Level 2+ 的配置项
        if (config.find("patterns") != std::string::npos ||
            config.find("caseSensitive") != std::string::npos) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                  "Configuration error: 'patterns' and 'caseSensitive' are not supported in Level 1",
                                  "Remove these fields or upgrade to Level 2+");
        }

        m_config = config;
        return {};
    }
} // namespace LangPlugins::RegexSplitter::Internal::V1
```

#### 9.4.4 Level 2 实现

```cpp
// internal/V2/TaskImpl.h
#ifndef LANGPLUGINS_REGEXSPLITER_INTERNAL_V2_TASKIMPL_H
#define LANGPLUGINS_REGEXSPLITER_INTERNAL_V2_TASKIMPL_H

#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal
{
    class TaskImplBase;
}

namespace LangPlugins::RegexSplitter::Internal::V2
{
    class RegexSplitterTaskImpl : public Internal::TaskImplBase {
    public:
        explicit RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        std::string m_config;
        std::vector<std::string> m_patterns;  // Level 2 新增：支持多个模式
    };
} // namespace LangPlugins::RegexSplitter::Internal::V2

#endif // LANGPLUGINS_REGEXSPLITER_INTERNAL_V2_TASKIMPL_H
```

```cpp
// internal/V2/TaskImpl.cpp
#include "TaskImpl.h"
#include <LangCore/Task/SplitterTask.h>
#include <LangCore/Support/Error.h>

namespace LangPlugins::RegexSplitter::Internal::V2
{
    RegexSplitterTaskImpl::RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> RegexSplitterTaskImpl::initialize() {
        // Level 2 的初始化逻辑
        auto cfg = LangCore::config(m_spec);

        // 配置验证：确保配置项与当前 Level 匹配
        // Level 2 支持 "patterns" 配置项，但不支持 "caseSensitive"
        if (cfg.has("caseSensitive")) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                  "Configuration error: 'caseSensitive' is not supported in Level 2",
                                  "Remove this field or upgrade to Level 3");
        }

        // 支持向后兼容：如果只有 "pattern"（Level 1 的配置），转换为 "patterns"
        if (cfg.has("pattern") && !cfg.has("patterns")) {
            auto pattern = cfg.getString("pattern");
            if (!pattern) {
                return pattern.takeError();
            }
            m_patterns.push_back(*pattern);
        } else {
            auto patterns = cfg.getStringArray("patterns");
            if (!patterns) {
                return patterns.takeError();
            }
            m_patterns = *patterns;
        }

        // 保存配置
        m_config = R"({"patterns": [)";
        for (size_t i = 0; i < m_patterns.size(); ++i) {
            if (i > 0) m_config += ",";
            m_config += "\"" + m_patterns[i] + "\"";
        }
        m_config += "]}";

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        // Level 2 的处理逻辑（支持多个模式）
        const auto splitterInput = input.as<LangCore::SplitterInputV1>();
        if (!splitterInput) {
            return LangCore::Error(LangCore::Error::RuntimeError,
                                  "Invalid input type for Level 2");
        }

        // 改进的分割逻辑
        auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
        // ... 使用 m_patterns 进行分割

        return result;
    }

    std::string RegexSplitterTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> RegexSplitterTaskImpl::setConfig(const std::string &config) {
        // Level 2 的配置更新逻辑
        // 配置验证：确保不包含 Level 3 的配置项
        if (config.find("caseSensitive") != std::string::npos) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                  "Configuration error: 'caseSensitive' is not supported in Level 2",
                                  "Remove this field or upgrade to Level 3");
        }

        m_config = config;
        // ... 解析 config 并更新 m_patterns
        return {};
    }
} // namespace LangPlugins::RegexSplitter::Internal::V2
```

#### 9.4.5 Level 3 实现

```cpp
// internal/V3/TaskImpl.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_V3_TASKIMPL_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_V3_TASKIMPL_H

#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal
{
    class TaskImplBase;
}

namespace LangPlugins::RegexSplitter::Internal::V3
{
    class RegexSplitterTaskImpl : public Internal::TaskImplBase {
    public:
        explicit RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        const LangCore::ModuleSpec *m_spec;
        std::string m_config;
        std::vector<std::string> m_patterns;
        bool m_caseSensitive = false;  // Level 3 新增：支持大小写敏感
    };
} // namespace LangPlugins::RegexSplitter::Internal::V3

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_V3_TASKIMPL_H
```

```cpp
// internal/V3/TaskImpl.cpp
#include "TaskImpl.h"
#include <LangCore/Task/SplitterTask.h>
#include <LangCore/Support/Error.h>

namespace LangPlugins::RegexSplitter::Internal::V3
{
    RegexSplitterTaskImpl::RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> RegexSplitterTaskImpl::initialize() {
        // Level 3 的初始化逻辑
        auto cfg = LangCore::config(m_spec);

        // 支持向后兼容：如果只有 "pattern"（Level 1 的配置），转换为 "patterns"
        if (cfg.has("pattern") && !cfg.has("patterns")) {
            auto pattern = cfg.getString("pattern");
            if (!pattern) {
                return pattern.takeError();
            }
            m_patterns.push_back(*pattern);
        } else {
            auto patterns = cfg.getStringArray("patterns");
            if (!patterns) {
                return patterns.takeError();
            }
            m_patterns = *patterns;
        }

        m_caseSensitive = cfg.getBool("caseSensitive", false);

        // 保存配置
        m_config = R"({"patterns": [)";
        for (size_t i = 0; i < m_patterns.size(); ++i) {
            if (i > 0) m_config += ",";
            m_config += "\"" + m_patterns[i] + "\"";
        }
        m_config += R"(], "caseSensitive": )" + std::string(m_caseSensitive ? "true" : "false") + "}";

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        // Level 3 的处理逻辑（支持大小写敏感）
        const auto splitterInput = input.as<LangCore::SplitterInputV1>();
        if (!splitterInput) {
            return LangCore::Error(LangCore::Error::RuntimeError,
                                  "Invalid input type for Level 3");
        }

        // 最优化的分割逻辑
        auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
        // ... 使用 m_patterns 和 m_caseSensitive 进行分割

        return result;
    }

    std::string RegexSplitterTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> RegexSplitterTaskImpl::setConfig(const std::string &config) {
        // Level 3 的配置更新逻辑
        m_config = config;
        // ... 解析 config 并更新 m_patterns 和 m_caseSensitive
        return {};
    }
} // namespace LangPlugins::RegexSplitter::Internal::V3
```

### 9.5 CMake 配置

#### 9.5.1 CMakeLists.txt 示例

```cmake
# plugins/Splitters/RegexSplitter/CMakeLists.txt

cmake_minimum_required(VERSION 3.19)

project(RegexSplitterPlugin)

# 收集所有源文件
set(SOURCES
    main.cpp
    Plugin.cpp
    Task.cpp
    internal/V1/TaskImpl.cpp
    internal/V2/TaskImpl.cpp
    internal/V3/TaskImpl.cpp
)

# 创建插件库
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# 设置输出名称
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "RegexSplitter"
)

# 链接依赖
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        LangCore
        stdcorelib
        nlohmann_json::nlohmann_json
)

# 设置包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
```

### 9.6 配置文件

#### 9.6.1 package.json 示例

```json
{
  "packageId": "regex-splitter-official",
  "version": "1.0.0",
  "vendor": "Language Manager Team",
  "description": "Regex-based text splitter with multi-level support",
  "modules": {
    "splitter": [
      {
        "moduleId": "splitter-regex",
        "class": "RegexSplitter.RegexSplitterPlugin",
        "configuration": "modules/regex-splitter/config.json",
        "pluginType": "CorePlugin",
        "requiredLevel": 1,
        "supportedLevels": [1, 2, 3]
      }
    ]
  }
}
```

#### 9.6.2 config.json 示例

```json
{
  "level": 2,
  "patterns": [
    "\\s+",
    "[,.;!?]+"
  ],
  "caseSensitive": false
}
```

### 9.7 向下兼容性保证

#### 9.7.1 Level 选择逻辑

```cpp
// Task 构造函数中的 Level 选择逻辑
RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec)
    : LangCore::Task(spec), _impl(std::make_unique<Impl>(spec)) {
    int level = spec->apiLevel();

    // 向下兼容：选择最高支持的 Level
    if (level >= 3) {
        _impl->impl = std::make_unique<Internal::V3::RegexSplitterTaskImpl>(spec);
    } else if (level >= 2) {
        _impl->impl = std::make_unique<Internal::V2::RegexSplitterTaskImpl>(spec);
    } else {
        // Level 1 作为默认（向下兼容）
        _impl->impl = std::make_unique<Internal::V1::RegexSplitterTaskImpl>(spec);
    }
}
```

#### 9.7.2 兼容性矩阵

| 配置 Level | 支持的 Level | 使用的实现 | 说明 |
|-----------|-------------|-----------|------|
| 1 | 1 | V1 | 使用 Level 1 实现 |
| 2 | 1, 2 | V2 | 使用 Level 2 实现（向下兼容 Level 1） |
| 3 | 1, 2, 3 | V3 | 使用 Level 3 实现（向下兼容 Level 1, 2） |

### 9.8 运行时配置更新

#### 9.8.1 配置更新限制

```cpp
LangCore::Expected<void> RegexSplitterTask::setConfig(const std::string &config) {
    // 配置更新只能在当前 Level 内进行
    // 不能改变 Level 等级

    int level = spec()->apiLevel();

    if (level >= 3) {
        auto *impl = static_cast<Internal::V3::RegexSplitterTaskImpl *>(_impl->impl.get());
        return impl->setConfig(config);
    } else if (level >= 2) {
        auto *impl = static_cast<Internal::V2::RegexSplitterTaskImpl *>(_impl->impl.get());
        return impl->setConfig(config);
    } else {
        auto *impl = static_cast<Internal::V1::RegexSplitterTaskImpl *>(_impl->impl.get());
        return impl->setConfig(config);
    }
}
```

**关键点**：
- 配置更新不能改变 Level 等级
- 只能更新当前 Level 内的配置项
- 如果需要使用更高 Level 的功能，必须重新加载插件

### 9.9 最佳实践

#### 9.9.1 新增 Level 版本

当需要新增 Level 版本时，遵循以下步骤：

1. **创建新的版本目录**：
   ```
   internal/V4/
   ├── TaskImpl.h
   └── TaskImpl.cpp
   ```

2. **实现新的版本类**：
   ```cpp
   namespace LangPlugins::RegexSplitter::Internal::V4 {
       class RegexSplitterTaskImpl {
           // ... Level 4 实现
       };
   }
   ```

3. **更新 Task.cpp**：
   - 添加前向声明
   - 更新构造函数中的 Level 选择逻辑
   - 更新所有方法中的路由逻辑

4. **更新 CMakeLists.txt**：
   - 添加新版本的源文件

5. **更新文档**：
   - 记录新 Level 的变化
   - 更新兼容性矩阵

#### 9.9.2 代码复用

为了避免代码重复，可以提取公共功能到辅助类：

```cpp
// internal/Common/SplitterHelper.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITTERHELPER_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITTERHELPER_H

#include <string>
#include <vector>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    class SplitterHelper {
    public:
        static std::vector<std::string> splitByPattern(
            const std::string &text,
            const std::string &pattern
        );

        static std::vector<std::string> splitByPatterns(
            const std::string &text,
            const std::vector<std::string> &patterns,
            bool caseSensitive = false
        );
    };
} // namespace LangPlugins::RegexSplitter::Internal::Common

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITTERHELPER_H
```

然后在各个版本实现中使用：

```cpp
// internal/V1/TaskImpl.cpp
#include "../Common/SplitterHelper.h"

namespace LangPlugins::RegexSplitter::Internal::V1
{
    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        // ... 使用 SplitterHelper
        auto segments = Common::SplitterHelper::splitByPattern(text, pattern);
        // ...
    }
}
```

#### 9.9.3 测试策略

为每个 Level 版本编写独立的测试：

```cpp
// tests/test_regex_splitter_v1.cpp
TEST(RegexSplitterV1, BasicSplit) {
    // Level 1 测试
}

// tests/test_regex_splitter_v2.cpp
TEST(RegexSplitterV2, MultiplePatterns) {
    // Level 2 测试
}

// tests/test_regex_splitter_v3.cpp
TEST(RegexSplitterV3, CaseSensitive) {
    // Level 3 测试
}
```

### 9.10 迁移指南

#### 9.10.1 从现有插件迁移

如果现有插件只支持单个 Level，迁移到多版本支持的步骤：

1. **创建 internal 目录**：
   ```
   mkdir -p internal/V1
   ```

2. **移动实现代码**：
   - 将现有的 Task 实现移动到 `internal/V1/TaskImpl.h` 和 `internal/V1/TaskImpl.cpp`
   - 更新命名空间为 `Internal::V1`

3. **创建 Task 基类**：
   - 创建新的 `Task.h` 和 `Task.cpp`
   - 实现路由逻辑

4. **更新 CMakeLists.txt**：
   - 更新源文件列表

5. **测试**：
   - 确保所有现有功能正常工作

#### 9.10.2 兼容性验证

迁移后，确保以下方面：

- ✅ 现有配置文件无需修改即可工作
- ✅ 现有插件接口保持不变
- ✅ 现有测试用例通过
- ✅ 向下兼容到 Level 1

### 9.11 代码复用机制（性能优化）

为了减少虚函数调用开销和代码重复，提取公共功能到 `Common` 命名空间：

```cpp
// internal/Common/SplitUtils.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H

#include <vector>
#include <string>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    /// 通用的分割工具类（避免虚函数调用）
    class SplitUtils {
    public:
        /// 使用单个模式分割文本
        static std::vector<std::string> splitWithPattern(
            const std::string &text,
            const std::string &pattern
        );

        /// 使用多个模式分割文本
        static std::vector<std::string> splitWithPatterns(
            const std::string &text,
            const std::vector<std::string> &patterns,
            bool caseSensitive = false
        );

        /// 验证正则表达式
        static LangCore::Expected<void> validateRegex(const std::string &pattern);

        /// 解析配置字符串
        static LangCore::Expected<std::vector<std::string>> parsePatterns(
            const std::string &config
        );
    };
} // namespace LangPlugins::RegexSplitter::Internal::Common

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H
```

**使用示例**：

```cpp
// 在 Level 1 实现中使用
LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
    const auto splitterInput = input.as<LangCore::SplitterInputV1>();
    if (!splitterInput) {
        return LangCore::Error(LangCore::Error::RuntimeError,
                              "Invalid input type for Level 1");
    }

    // 使用公共工具类（避免重复代码）
    auto cfg = LangCore::config(m_spec);
    auto pattern = cfg.getString("pattern");
    if (!pattern) {
        return pattern.takeError();
    }

    auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
    result->splitterResult = Common::SplitUtils::splitWithPattern(
        splitterInput->splitterInput,
        *pattern
    );

    return result;
}

// 在 Level 3 实现中使用
LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
    const auto splitterInput = input.as<LangCore::SplitterInputV1>();
    if (!splitterInput) {
        return LangCore::Error(LangCore::Error::RuntimeError,
                              "Invalid input type for Level 3");
    }

    // 使用公共工具类（避免重复代码）
    auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
    result->splitterResult = Common::SplitUtils::splitWithPatterns(
        splitterInput->splitterInput,
        m_patterns,
        m_caseSensitive
    );

    return result;
}
```

### 9.12 Level 版本兼容性检查

为了确保不同 Level 之间的配置可以平滑迁移，添加配置迁移机制：

```cpp
// internal/Common/ConfigMigration.h
#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H

#include <string>
#include <nlohmann/json.hpp>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    /// 配置迁移工具类
    class ConfigMigration {
    public:
        /// 从 Level 1 配置迁移到 Level 2+
        static nlohmann::json migrateFromLevel1(
            const nlohmann::json &config,
            int targetLevel
        );

        /// 从 Level 2 配置迁移到 Level 3+
        static nlohmann::json migrateFromLevel2(
            const nlohmann::json &config,
            int targetLevel
        );

        /// 验证配置是否兼容目标 Level
        static LangCore::Expected<void> validateConfigForLevel(
            const nlohmann::json &config,
            int targetLevel
        );

        /// 获取迁移建议
        static std::string getMigrationSuggestion(
            const nlohmann::json &config,
            int currentLevel,
            int targetLevel
        );
    };
} // namespace LangPlugins::RegexSplitter::Internal::Common

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H
```

**使用示例**：

```cpp
// 在 Level 2 实现中使用配置迁移
LangCore::Expected<void> RegexSplitterTaskImpl::initialize() {
    auto cfg = LangCore::config(m_spec);

    // 检查是否需要迁移
    auto jsonConfig = nlohmann::json::parse(cfg.raw().dump());

    // 如果配置来自 Level 1，自动迁移到 Level 2
    if (cfg.has("pattern") && !cfg.has("patterns")) {
        auto migratedConfig = Common::ConfigMigration::migrateFromLevel1(
            jsonConfig,
            2  // 目标 Level
        );
        // 更新配置
        auto patterns = migratedConfig["patterns"].get<std::vector<std::string>>();
        m_patterns = patterns;
    } else {
        auto patterns = cfg.getStringArray("patterns");
        if (!patterns) {
            return patterns.takeError();
        }
        m_patterns = *patterns;
    }

    return {};
}
```

### 9.13 性能优化

为了减少运行时开销，采用以下优化策略：

1. **构造函数中只选择一次**：Level 选择逻辑在构造函数中只执行一次，避免每次方法调用都检查 Level

2. **使用类型安全的接口**：通过 `TaskImplBase` 接口，避免 `static_cast` 的运行时开销

3. **编译时优化**：使用 `inline` 和 `constexpr` 优化公共函数

4. **避免虚函数调用**：通过 `Common` 命名空间的静态函数，避免虚函数调用开销

```cpp
// 性能优化示例
class RegexSplitterTask::Impl {
public:
    const LangCore::ModuleSpec *spec = nullptr;
    std::unique_ptr<Internal::TaskImplBase> impl;  // 类型安全的接口
    Impl(const LangCore::ModuleSpec *s) : spec(s) {}
};

// 构造函数中只选择一次（运行时开销最小）
RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec)
    : LangCore::Task(spec), _impl(std::make_unique<Impl>(spec)) {
    int level = spec->apiLevel();

    // 错误处理：检查 Level 范围
    if (level < 1 || level > 3) {
        auto errorMsg = std::string("Plugin only supports Level 1-3, but requested Level ") +
                       std::to_string(level);
        auto suggestion = "Please use Level 1, 2, or 3. If you need higher Level, " +
                         "consider upgrading the plugin.";

        throw std::runtime_error(errorMsg + ". " + suggestion);
    }

    // 选择实现（只执行一次）
    if (level >= 3) {
        _impl->impl = std::make_unique<Internal::V3::RegexSplitterTaskImpl>(spec);
    } else if (level >= 2) {
        _impl->impl = std::make_unique<Internal::V2::RegexSplitterTaskImpl>(spec);
    } else {
        // Level 1 作为默认（向下兼容）
        _impl->impl = std::make_unique<Internal::V1::RegexSplitterTaskImpl>(spec);
    }
}

// 方法实现简化（直接调用，无需检查 Level，无运行时开销）
LangCore::Expected<void> RegexSplitterTask::initialize() {
    return _impl->impl->initialize();
}

LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
RegexSplitterTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
    return _impl->impl->start(input);
}
```

### 9.14 总结

多版本插件代码组织方案的核心要点：

1. **文件组织清晰**：通过 `internal/Vx/` 目录结构清晰地区分不同版本
2. **命名空间隔离**：使用 `Internal::Vx` 命名空间隔离不同版本的实现
3. **路由逻辑统一**：Task 基类统一处理版本选择和路由
4. **对外接口稳定**：Plugin 和 Task 的公共接口保持不变
5. **向下兼容保证**：Level 1 作为默认实现，确保兼容性
6. **运行时稳定**：配置更新不改变 Level 等级
7. **易于扩展**：新增 Level 版本只需添加新的实现类
8. **代码复用**：通过公共辅助类避免代码重复

这个方案满足"Write Once, Run Forever"的设计理念，确保插件系统在 API 演进的同时保持长期稳定性。

---

## 10. 核心数据结构

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

**文档版本**: 1.1
**最后更新**: 2026-04-02
**更新内容**:
- 添加多版本插件代码组织方案（第9节）
- 删除废案和历史记录（原第12节和第13节）
- 更新文档版本号
        return LangCore::NO<MyTask>::create(spec);
    }
};

LANGCORE_EXPORT_PLUGIN(MyPlugin)

// 改进后（2 行代码）
LANGCORE_DECLARE_TASK_PLUGIN(MyPlugin, MyTask, "g2p", "my-plugin")
LANGCORE_EXPORT_TASK_PLUGIN(MyPlugin)
```

**实现要点**：
- 保持原有的插件开发方式不变
- 新增辅助宏作为便捷选项
- 宏展开后生成标准的插件代码
- 不影响插件系统的灵活性

**方案 6：提供插件开发模板**

```cpp
// 新增模板基类
template <typename TaskClass>
class SimpleTaskPlugin : public LangCore::TaskPlugin {
public:
    SimpleTaskPlugin(const std::string &category, const std::string &key)
        : m_category(category), m_key(key) {}

    int apiLevel() const override { return 1; }
    const char *key() const override { return m_key.c_str(); }

    LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(
        const LangCore::ModuleSpec *spec
    ) override {
        return LangCore::NO<TaskClass>::create(spec);
    }

private:
    std::string m_category;
    std::string m_key;
};

// 使用示例
// 当前（自定义插件类）
class MyPlugin final : public LangCore::TaskPlugin {
    // ... 实现
};

// 改进后（使用模板）
using MyPlugin = SimpleTaskPlugin<MyTask>;
LANGCORE_EXPORT_PLUGIN(MyPlugin)
```

**实现要点**：
- 保持原有的插件开发方式不变
- 新增模板类作为便捷选项
- 模板类自动实现标准插件接口
- 允许高级用户继续自定义

#### 12.3.4 配置管理优化方案

**方案 7：提供类型安全的配置接口**

```cpp
// 新增类型安全的配置方法
class Task {
public:
    // 类型安全的配置接口
    template <typename ConfigType>
    ConfigType getConfig() const;

    template <typename ConfigType>
    Expected<void> setConfig(const ConfigType &config);
};

// 配置结构示例
struct G2pConfig {
    bool enabled = true;
    std::string dictPath;
    double threshold = 0.5;
    
    // 序列化为 JSON
    nlohmann::json toJson() const {
        return {
            {"enabled", enabled},
            {"dictPath", dictPath},
            {"threshold", threshold}
        };
    }
    
    // 从 JSON 反序列化
    static G2pConfig fromJson(const nlohmann::json &j) {
        G2pConfig config;
        config.enabled = j.value("enabled", true);
        config.dictPath = j.value("dictPath", "");
        config.threshold = j.value("threshold", 0.5);
        return config;
    }
};

// 使用示例
// 当前（JSON 字符串）
std::string config = R"({"enabled": true, "dictPath": "path"})";
task->setConfig(config);

// 改进后（类型安全）
G2pConfig config;
config.enabled = true;
config.dictPath = "path";
task->setConfig(config);

// 获取配置
auto currentConfig = task->getConfig<G2pConfig>();
```

**实现要点**：
- 保持原有的 JSON 字符串接口不变
- 新增类型安全的模板方法作为包装器
- 配置结构需要提供 `toJson()` 和 `fromJson()` 方法
- 内部自动处理 JSON 序列化和反序列化

### 12.4 实施建议

#### 12.4.1 实施优先级

**高优先级（立即实施）**：
1. **方案 1**：便捷初始化方法 - 大幅降低使用门槛
2. **方案 3**：统一文本转换接口 - 简化最常见的使用场景
3. **方案 4**：简化依赖注入 - 降低插件开发复杂度

**中优先级（短期实施）**：
4. **方案 2**：简化任务获取接口 - 提升代码可读性
5. **方案 7**：类型安全配置接口 - 提升配置安全性

**低优先级（长期优化）**：
6. **方案 5**：插件开发辅助宏 - 代码糖，非必需
7. **方案 6**：插件开发模板 - 高级功能，非必需

#### 12.4.2 实施步骤

**阶段 1：便捷接口实现（1-2 周）**
1. 在 `Manager` 类中添加便捷初始化方法
2. 在 `Manager` 类中添加统一文本转换接口
3. 在 `Task` 类中添加依赖获取辅助方法
4. 编写单元测试和文档
5. 更新 README 和使用示例

**阶段 2：配置优化（1 周）**
1. 在 `Task` 类中添加类型安全配置方法
2. 定义常用配置结构（G2pConfig、SplitterConfig 等）
3. 编写单元测试和文档
4. 更新配置相关文档

**阶段 3：插件开发工具（1 周）**
1. 实现插件开发辅助宏
2. 实现插件开发模板
3. 创建插件示例和教程
4. 更新插件开发文档

#### 12.4.3 兼容性保证

**向后兼容策略**：
1. 所有新方法都是附加的，不修改现有方法签名
2. 现有代码无需修改即可继续工作
3. 新文档标注推荐使用新 API，但旧 API 仍然支持
4. 提供迁移指南，帮助用户逐步迁移到新 API

**弃用策略**：
1. 不立即弃用任何现有 API
2. 在文档中标注某些 API 为"不推荐使用"
3. 在未来的主要版本（Level 变更）中考虑移除
4. 提供弃用警告机制（可选）

### 12.5 效果评估

#### 12.5.1 代码简化效果

**初始化流程**：
- **当前**：5 个手动步骤，约 20 行代码
- **改进后**：1 个步骤，1 行代码
- **简化比例**：80%

**文本转换**：
- **当前**：4 个步骤，约 10 行代码（含内存管理）
- **改进后**：1 个步骤，1 行代码
- **简化比例**：90%

**依赖获取**：
- **当前**：3 层查找，3 行代码
- **改进后**：1 层调用，1 行代码
- **简化比例**：67%

**插件开发**：
- **当前**：10+ 行代码，需要理解 8 个核心类
- **改进后**：2 行代码（使用辅助宏），需要理解 2 个核心类
- **简化比例**：80%

#### 12.5.2 学习曲线改善

**新手用户**：
- **当前**：需要理解插件系统、包管理、依赖注入等多个概念
- **改进后**：可以从简单 API 开始，逐步学习高级功能
- **改善**：学习时间从数天降低到数小时

**中级用户**：
- **当前**：可以完成基本任务，但遇到复杂场景时需要深入研究
- **改进后**：可以更高效地完成常见任务
- **改善**：开发效率提升 50%+

**高级用户**：
- **当前**：可以完全控制所有细节
- **改进后**：保持完全控制能力，同时可以使用便捷接口
- **改善**：不影响，反而增加了选择灵活性

#### 12.5.3 代码质量改善

**可读性**：
- 新 API 更加直观，减少了样板代码
- 方法命名更加语义化
- 类型安全接口减少了错误可能性

**可维护性**：
- 减少了包装层，代码路径更短
- 统一的错误处理方式
- 更好的文档和示例

**可扩展性**：
- 保持了插件系统的灵活性
- 新功能可以作为便捷接口添加
- 不影响现有的扩展机制

### 12.6 注意事项

1. **保持插件化原则**：所有改进必须保持插件系统的完整性，不能简化到破坏插件化的程度

2. **向后兼容**：新 API 是对旧 API 的补充，而不是替换。必须保证现有代码无需修改即可继续工作。

3. **渐进式改进**：不要试图一次性重构所有代码。采用渐进式改进策略，逐步引入新 API。

4. **文档同步**：每次添加新 API 都要同步更新文档和示例代码，确保用户能够理解和使用。

5. **性能考虑**：便捷接口不应该引入显著的性能开销。避免不必要的拷贝和类型转换。

6. **测试覆盖**：所有新 API 都要有完整的单元测试和集成测试，确保功能的正确性。

7. **社区反馈**：在实施过程中收集用户反馈，根据实际使用情况调整方案。

---

**文档结束**