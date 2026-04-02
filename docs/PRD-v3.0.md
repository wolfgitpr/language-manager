# Language Manager 产品需求文档

## 文档说明

本文档定义 Language Manager 的核心设计规范和开发指南。

**核心目标**：实现长期免维护的第三方插件加载系统，遵循"Write Once, Run Forever"的设计理念。

**版本**：5.2
**日期**：2026-04-02
**更新内容**：
- 简化架构设计，移除责任链相关内容
- 删除冗余和无效部分，保持简洁可靠
- 新增"目录结构设计原则"章节，明确 core 目录的设计规范
- 详细说明各子目录的职责和依赖关系
- 将 LevelCompatibilityChecker 从 Support 移到 Module/Dependency/ 目录
- 统一依赖和兼容性管理功能，提高内聚性

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
- 示例：Splitter、Tagger、G2p、Driver

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
- `updateConfig(config)`：更新配置

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

### 3.1 插件接口

**TaskPlugin**：任务工厂插件

```cpp
class TaskPlugin : public FactoryPlugin {
public:
    virtual int apiLevel() const = 0;
    virtual const char *key() const = 0;
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

**SessionFactoryPlugin**：会话工厂插件（AI 模型驱动）

```cpp
class SessionFactoryPlugin : public FactoryPlugin {
public:
    virtual Expected<NO<Task>> createSession(
        const std::filesystem::path &path,
        const NO<TaskInitArgs> &args
    ) = 0;
};
```

### 3.2 插件注册

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

### 3.3 包格式

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

### 3.4 依赖声明

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

### 4.1 Level 兼容性规则

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

### 4.2 LevelCompatibilityChecker

```cpp
class LevelCompatibilityChecker {
public:
    struct LevelCompatibilityResult {
        bool isCompatible;
        int pluginLevel;
        int systemCurrentLevel;
        int systemMinimumLevel;
        int systemMaximumLevel;
        std::string message;
        std::string suggestion;

        bool isInSupportedRange() const;
    };

    /// 检查核心插件的 Level 兼容性
    /// @param pluginLevel 插件 Level
    /// @param currentLevel 系统 Level
    /// @param maximumLevel 系统最大支持 Level（0 表示无限制，使用 currentLevel）
    /// @param minimumLevel 系统最小支持 Level
    /// @return 兼容性检查结果
    static LevelCompatibilityResult checkCorePlugin(
        int pluginLevel,
        int currentLevel,
        int maximumLevel,
        int minimumLevel
    );

    /// 检查依赖插件的 Level 兼容性
    /// @param pluginLevel 依赖插件 Level
    /// @param currentLevel 系统 Level
    /// @param maximumLevel 系统最大支持 Level（0 表示无限制，使用 currentLevel）
    /// @param minimumLevel 系统最小支持 Level
    /// @return 兼容性检查结果
    static LevelCompatibilityResult checkDependencyPlugin(
        int pluginLevel,
        int currentLevel,
        int maximumLevel,
        int minimumLevel
    );

    /// 批量检查所有插件和依赖
    /// @param pluginLevels 插件 Level 列表
    /// @param dependencyLevels 依赖 Level 列表
    /// @param currentLevel 系统 Level
    /// @param maximumLevel 系统最大支持 Level（0 表示无限制，使用 currentLevel）
    /// @param minimumLevel 系统最小支持 Level
    /// @return 所有检查结果
    static std::vector<LevelCompatibilityResult> checkAll(
        const std::vector<std::pair<std::string, int>> &pluginLevels,
        const std::vector<std::pair<std::string, int>> &dependencyLevels,
        int currentLevel,
        int maximumLevel,
        int minimumLevel
    );

    /// 生成兼容性检查报告
    /// @param results 检查结果列表
    /// @return 格式化的报告字符串
    static std::string generateReport(
        const std::vector<LevelCompatibilityResult> &results
    );
};
```

### 4.3 依赖解析

**DependencyResolver**：解析模块依赖关系，计算初始化顺序

**DependencyGraph**：构建依赖关系图，检测循环依赖，计算初始化顺序

---

## 6. 插件开发规范

### 5.1 插件类型判断规则

**核心原则**：任何使用 Core 声明的结构体（如 TaskInput、TaskResult、Expected 等）的插件都是核心插件，必须进行 Level 检查。

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

### 5.3 SessionFactoryPlugin 开发规范

**基本结构**：

```cpp
#include <LangCore/Task/SessionTask.h>

namespace LangPlugins::MyDriver
{
    class MySessionFactoryPlugin final : public LangCore::SessionFactoryPlugin {
    public:
        MySessionFactoryPlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override {
            return "driver.my-driver";
        }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<MySessionTask>::create(spec);
        }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createSession(
            const std::filesystem::path &path,
            const LangCore::NO<LangCore::TaskInitArgs> &args
        ) override {
            return LangCore::NO<MySessionTask>::create(spec);
        }
    };
}

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyDriver::MySessionFactoryPlugin)
```

---

## 7. Task 开发规范

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

### 6.2 自定义 Input 和 Result

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

### 6.3 SessionTask 开发规范

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

### 7.1 基本原则

**简洁可靠**：
- 遇到错误时直接返回 `Expected<T>` 错误
- 不设计重试机制
- 不设计回滚机制
- 错误信息应该清晰、具体
- 使用 Logger 记录关键操作和错误信息

### 7.2 错误返回

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

### 7.3 常见错误类型

- `Error::InvalidArgument` - 无效参数
- `Error::InvalidFormat` - 无效格式
- `Error::FileNotFound` - 文件未找到
- `Error::RuntimeError` - 运行时错误
- `Error::InitError` - 初始化错误

### 7.4 日志记录

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

## 9. 构建配置

### 9.1 构建环境

**操作系统**：
- Windows 10/11 (MSVC 2019/2022)
- Linux (GCC 9+)
- macOS (Clang 11+)

**必需工具**：
- CMake 3.19+
- vcpkg (依赖管理)
- Qt 6.7.3
- qmsetup (Qt 构建工具)

**C++ 标准**：
- C++17 或更高

### 9.2 构建目录说明

项目使用多个构建目录，各有不同用途：

| 目录 | 用途 | 说明 |
|------|------|------|
| `build/` | AI 自动编译测试 | 专用于 AI 修改代码时的自动编译测试和修复编译错误，不影响其他构建目录 |
| `cmake-build-debug/` | CLion Debug 构建 | CLion IDE 的 Debug 模式构建目录 |
| `cmake-build-release/` | CLion Release 构建 | CLion IDE 的 Release 模式构建目录 |
| `install/` | 安装目录 | CMake 安装目标输出目录 |

**注意**：`build/` 目录专用于 AI 修改代码时的自动编译测试和修复编译错误，不会影响其他构建目录（如 `cmake-build-debug/` 和 `cmake-build-release/`）。

### 9.3 Visual Studio 2026 构建配置

**工具链**：
- 编译器：Visual Studio 2026 (MSVC 19.50.35727.0)
- 路径：`D:\Programs\vs2026`

**CMake 配置命令**：

```bash
# 设置 Visual Studio 环境并运行 CMake 配置
cmd /c '"D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake -S . -B build -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE="D:\projects\ds-editor-lite\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_PREFIX_PATH="D:\Programs\qt5\6.10.2\msvc2022_64" -DCMAKE_INSTALL_PREFIX="install" -DLANGMGR_BUILD_PLUGINS=ON -DLANGMGR_BUILD_TESTS=ON -DLANGPLUGINS_ENABLE_DIRECTML=ON'
```

**CMake 参数说明**：

| 参数 | 值 | 说明 |
|------|-----|------|
| `-G` | `NMake Makefiles` | 使用 NMake 生成器 |
| `-DCMAKE_TOOLCHAIN_FILE` | `D:\projects\ds-editor-lite\vcpkg\scripts\buildsystems\vcpkg.cmake` | vcpkg 工具链文件 |
| `-DCMAKE_PREFIX_PATH` | `D:\Programs\qt5\6.10.2\msvc2022_64` | Qt 前缀路径 |
| `-DCMAKE_INSTALL_PREFIX` | `install` | 安装目录 |
| `-DLANGMGR_BUILD_PLUGINS` | `ON` | 构建插件 |
| `-DLANGMGR_BUILD_TESTS` | `ON` | 构建测试 |
| `-DLANGPLUGINS_ENABLE_DIRECTML` | `ON` | 启用 DirectML 支持 |

**构建命令**：

```bash
# 使用 NMake 构建（Debug 模式）
cmd /c '"D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config Debug'

# 使用 NMake 构建（Release 模式）
cmd /c '"D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config Release'
```

### 9.4 依赖管理

**核心依赖**（通过 vcpkg 安装）：
- stdcorelib：提供智能指针、文件系统等基础设施
- nlohmann_json：JSON 处理库

**运行时依赖**：
- Qt 6.7.3：通过 qmsetup 集成
- ONNX Runtime：AI 模型推理框架
- cpp-pinyin：普通话拼音转换
- cpp-kana：日语假名转换

**依赖安装命令**：

```bash
# 使用 vcpkg 安装必需的依赖
vcpkg install stdcorelib
vcpkg install nlohmann-json
```

### 9.5 构建输出

**构建输出目录**：`build/out-amd64-Debug/`

**核心库**：
- `bin/LangCored.dll` - 核心库动态链接库
- `lib/LangCored.lib` - 核心库导入库

**测试程序**：
- `bin/tst_langCore.exe` - 核心库测试可执行文件

**插件**：
1. **Drivers/OnnxDriver** - ONNX Runtime 驱动插件
2. **Splitters/RegexTagger** - 正则表达式文本分割器
3. **Taggers/TemplateTagger** - 模板语言标记器
4. **G2ps** - G2p 处理器插件组：
   - CantoneseG2p - 粤语 G2p
   - LstmG2p - LSTM G2p
   - MandarinG2p - 普通话 G2p
   - TemplateG2p - 模板 G2p

**工具库**：
- `lib/InferUtild.lib` - 推理工具库
- `lib/OnnxUtild.lib` - ONNX 工具库

### 9.6 CLion 配置

**CMake 选项**（CLion 设置）：

```cmake
-DCMAKE_TOOLCHAIN_FILE=D:\projects\ds-editor-lite/vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.2\msvc2022_64
-DCMAKE_INSTALL_PREFIX=install
-DLANGMGR_BUILD_PLUGINS=ON
-DLANGMGR_BUILD_TESTS=ON
-DLANGPLUGINS_ENABLE_DIRECTML=ON
```

**工具链**：`D:\Programs\vs2026`

**构建类型**：
- Debug：`cmake-build-debug/`
- Release：`cmake-build-release/`

---

## 10. 设计简化建议

### 10.1 Task 配置接口设计

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

    // 更新配置（JSON 字符串，合并到现有配置）
    virtual Expected<void> updateConfig(const std::string &config) = 0;
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

// 更新配置（合并）
std::string update = R"({
    "param1": 200
})";
task->updateConfig(update);

// 获取配置
std::string currentConfig = task->getConfig();
```

**实现示例**：

```cpp
Expected<void> MyTask::updateConfig(const std::string &config) {
    try {
        auto newConfig = nlohmann::json::parse(config);
        m_config.merge_patch(newConfig);
        return {};
    } catch (const std::exception &e) {
        return Error(Error::InvalidFormat, std::string("Failed to parse config: ") + e.what());
    }
}
```

**注意事项**：
- 配置格式必须是合法的 JSON
- Task 内部使用 JSON 库解析配置（推荐 nlohmann_json）
- `updateConfig()` 应该合并配置而不是完全替换
- 配置验证由 Task 内部负责

### 10.2 简化 ManagerConfig

**问题**：
- 提供了过多的策略选项（CompatibilityPolicy、ErrorHandlingPolicy）
- 增加了配置复杂度

**建议**：
- 移除 `CompatibilityPolicy` 和 `ErrorHandlingPolicy`
- 默认使用 `Strict` 模式和 `LogAndSkip` 模式

**示例**：

```cpp
// 当前设计（过度复杂）
struct ManagerConfig {
    int currentLevel = 1;
    int minimumLevel = 1;
    int maximumLevel = 0;
    std::string currentVersion;

    enum class CompatibilityPolicy { Strict, Lenient, Adaptive };
    enum class ErrorHandlingPolicy { FailFast, ContinueOnError, LogAndSkip };

    CompatibilityPolicy compatibilityPolicy = CompatibilityPolicy::Strict;
    ErrorHandlingPolicy errorHandlingPolicy = ErrorHandlingPolicy::LogAndSkip;
};

// 简化后（移除策略选项）
struct ManagerConfig {
    int currentLevel = 1;
    int minimumLevel = 1;
    int maximumLevel = 0;
    std::string currentVersion;
    // 默认使用 Strict 模式和 LogAndSkip 模式
};
```

**收益**：
- 简化配置
- 减少配置错误的可能性
- 提高代码可维护性

### 10.3 统一错误类型

**问题**：
- 错误类型过多（13 种）
- 部分错误类型重复或过于细分

**建议**：
- 统一错误类型，减少错误类型的数量
- 提供更详细的错误信息（包含上下文信息）

**示例**：

```cpp
// 当前设计（错误类型过多）
enum Type {
    NoError = 0,
    InvalidFormat,
    FileNotFound,
    FileNotOpen,
    FileDuplicated,
    RecursiveDependency,
    FeatureNotSupported,
    InvalidArgument,
    NotImplemented,
    SessionError,
    TaskError,
    InterpreterNotFound,
    RuntimeError,
    NotInitialized
};

// 简化后（统一错误类型）
enum Type {
    Success = 0,
    ConfigError,         // 配置错误（InvalidFormat, InvalidArgument）
    FileSystemError,     // 文件系统错误（FileNotFound, FileNotOpen, FileDuplicated）
    DependencyError,     // 依赖错误（RecursiveDependency, InterpreterNotFound）
    RuntimeError,        // 运行时错误（SessionError, TaskError, RuntimeError）
    NotImplementedError, // 未实现错误（FeatureNotSupported, NotImplemented）
    InitializationError  // 初始化错误（NotInitialized）
};
```

**收益**：
- 简化错误处理逻辑
- 提高错误处理的一致性
- 减少错误类型的数量

---

## 11. 简洁设计原则

### 11.1 核心原则

**简洁可靠**：
- 遇到错误时直接返回 `Expected<T>` 错误
- 不设计重试机制
- 不设计回滚机制
- 错误信息应该清晰、具体
- 使用 Logger 记录关键操作和错误信息

**插件分类**：
- 核心插件（使用 Core 结构体）
- 工具插件（独立功能）

**统一规则**：
- Level 作为 API 兼容性的唯一标准
- 配置使用 JSON 字符串格式
- Task 内部自行解析配置

### 11.2 最佳实践

1. **错误处理**：
   - 遇到错误时直接返回 `Expected<T>` 错误
   - 不设计重试机制
   - 不设计回滚机制
   - 使用 Logger 记录关键操作和错误信息

2. **配置管理**：
   - 使用 JSON 格式定义配置
   - **推荐使用 ConfigAccessor**：`auto cfg = LangCore::config(spec())`
   - 必需字段使用 `Expected<T>` 返回类型的方法（如 `getString()`）
   - 可选字段使用带默认值的方法（如 `getString(key, defaultValue)`）
   - 支持配置热更新（通过 `setConfig`）
   - 配置验证由 Task 内部负责

3. **插件开发**：
   - 插件加载后常驻内存，无需复杂生命周期管理
   - 避免不必要的抽象层
   - 优先保证代码简洁性

4. **日志记录**：
   - 使用现有的日志系统
   - 记录关键操作和错误信息
   - 日志级别：Trace、Debug、Info、Success、Warning、Critical、Fatal

---

## 12. 设计演进路线图

### 12.1 短期目标（简化阶段）

**优先级 P0（必须完成）**：
- 简化 ManagerConfig（移除策略选项）
- 统一错误类型（减少错误类型的数量）
- 明确 Task 配置接口设计（JSON 字符串格式）

**优先级 P1（建议完成）**：
- 更新 Task 配置 API 文档
- 添加配置解析示例

### 12.2 长期目标（稳定阶段）

**稳定性和可靠性**：
- 完善单元测试
- 添加集成测试

**文档完善**：
- 更新 API 文档
- 添加最佳实践文档
- 添加故障排查指南

---

**文档结束**