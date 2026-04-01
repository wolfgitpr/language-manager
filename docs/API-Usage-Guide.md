# Language Manager API 使用指南

## 目录

1. [概述](#概述)
2. [核心概念](#核心概念)
3. [Level兼容性管理](#level兼容性管理)
4. [插件开发指南](#插件开发指南)
5. [API参考](#api参考)
6. [最佳实践](#最佳实践)
7. [故障排除](#故障排除)

## 概述

Language Manager 提供了强大的插件化架构，支持第三方自定义插件的开发和加载。本指南将帮助您：

- 理解Level兼容性机制
- 开发兼容的插件
- 使用Level兼容性管理功能
- 诊断和解决兼容性问题

## 核心概念

### 1. Level（API级别）

Level是Language Manager的API兼容性标准，是一个整数：

```
Level 1, Level 2, Level 3, ...
```

**重要说明**：
- Level是**唯一**的API兼容性判断标准
- Level定义了API的特性和功能集
- 不同Level的API可能不兼容

### 2. Version（版本号）

Version用于标识插件的版本，遵循语义化版本规范：

```
MAJOR.MINOR.PATCH
```

**Version的作用**：
- **不参与**API兼容性检查
- 仅用于Bug修复记录
- 用于依赖声明中的版本范围要求

**Version与Level的关系**：
- 建议：Version第一位与Level相同
- 示例：Level=1, Version=1.2.3

### 3. Level兼容性规则

#### 核心插件（Splitter/Tagger/G2p）

| Manager Level | Plugin Level | 兼容性 | 说明 |
|---------------|--------------|--------|------|
| 2 | 2 | ✅ 兼容 | 同级别 |
| 2 | 1 | ✅ 兼容 | 向下兼容一代 |
| 2 | 0 | ❌ 不兼容 | 低于Manager超过1代 |
| 2 | 3 | ❌ 不兼容 | 高于Manager |

**兼容性规则**：
```
Manager Level = M
Plugin Level = P

兼容条件: M - 1 <= P <= M
```

#### 依赖插件（Driver等）

依赖插件（如ONNX Driver）**不对Level进行限制**，通过dependency系统自动分析。

### 4. 插件类别

```cpp
enum class PluginCategory {
    CorePlugin,       // 核心插件：Splitter/Tagger/G2p
    DependencyPlugin  // 依赖插件：Driver等
};
```

## Level兼容性管理

### 1. 检查插件Level兼容性

```cpp
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>

using namespace LangCore;

// 定义系统Level配置
int currentLevel = 2;    // 系统 Level
int maximumLevel = 2;    // 系统最大支持 Level（0 表示无限制）
int minimumLevel = 1;    // 系统最小支持 Level

// 检查核心插件
int pluginLevel = 1;     // 插件 Level

auto result = LevelCompatibilityChecker::checkCorePlugin(
    pluginLevel,     // 插件 Level
    currentLevel,    // 系统 Level
    maximumLevel,    // 系统最大支持 Level
    minimumLevel     // 系统最小支持 Level
);

// 检查结果
if (result.isCompatible) {
    std::cout << "✅ Plugin is compatible!\n";
    std::cout << result.message << "\n";
} else {
    std::cout << "❌ Plugin is not compatible:\n";
    std::cout << result.message << "\n";
    std::cout << "Suggestion: " << result.suggestion << "\n";
}
```

### 2. 批量检查多个插件

```cpp
// 定义插件Level列表
std::vector<std::pair<std::string, int>> corePlugins = {
    {"g2p-cmn", 1},
    {"splitter-cmn", 1},
    {"tagger-cmn", 1}
};

// 定义依赖插件Level列表
std::vector<std::pair<std::string, int>> dependencyPlugins = {
    {"onnx-driver", 1}
};

// 批量检查所有插件
auto results = LevelCompatibilityChecker::checkAll(
    corePlugins,
    dependencyPlugins,
    currentLevel,
    maximumLevel,
    minimumLevel
);

// 生成兼容性报告
std::string report = LevelCompatibilityChecker::generateReport(results);
std::cout << report;
```

### 3. Level管理器

```cpp
#include <LangCore/Support/LevelCompatibility.h>

using namespace LangCore;

// 获取当前Manager Level
int managerLevel = LevelManager::getCurrentManagerLevel();

// 检查Level是否有效
bool isValid = LevelManager::isValidLevel(2);  // true

// 获取支持的Plugin Level范围
auto [minLevel, maxLevel] = LevelManager::getSupportedPluginLevelRange();
// 对于Manager Level=2: minLevel=1, maxLevel=2

// 检查插件类别是否需要Level检查
bool needsCheck = LevelManager::requiresLevelCheck(PluginCategory::CorePlugin);
// true for CorePlugin, false for DependencyPlugin
```

## 插件开发指南

### 1. 创建基础插件

```cpp
#include <LangCore/Task/Task.h>

using namespace LangCore;

class MyG2pTask : public Task {
public:
    explicit MyG2pTask(const ModuleSpec* spec) : Task(spec) {}

    // 返回插件的Level
    int apiLevel() const override {
        return 1;  // Level 1
    }

    Expected<void> initialize() override {
        // 初始化逻辑
        return {};
    }

    Expected<NO<TaskResult>> start(const NO<TaskInput>& input) override {
        // 执行逻辑
        auto result = NO<G2pResultV1>::create();
        // ... 填充结果
        return result;
    }
};
```

### 2. 插件导出

```cpp
#include <LangCore/Core/Plugin.h>
#include <LangCore/Task/TaskPlugin.h>

using namespace LangCore;

class MyPlugin : public TaskPlugin {
public:
    const char* key() const override { return "MyPlugin"; }

    Expected<NO<Task>> createTask(const ModuleSpec* spec) override {
        try {
            auto task = new MyG2pTask(spec);
            return NO<Task>(task);
        } catch (const std::exception& e) {
            return Error(Error::RuntimeError, e.what());
        }
    }
};

// 导出插件
LANGCORE_EXPORT_PLUGIN(MyPlugin)
```

### 3. 插件配置文件

创建 `plugin.json`:

```json
{
  "packageId": "my-plugin",
  "version": "1.2.3",
  "vendor": "Your Name",
  "copyright": "Copyright (C) 2025 Your Name",
  "description": "My custom G2p plugin",
  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-my",
        "class": "MyPlugin",
        "configuration": "config.json",
        "dependencies": []
      }
    ]
  }
}
```

### 4. 模块配置文件

创建 `config.json`:

```json
{
  "$version": "1.0",
  "level": 1,
  "name": "My G2p Module",
  "configuration": {
    "dictPath": "dict/",
    "modelPath": "model/"
  }
}
```

**重要**：
- `level` 字段定义了模块的API级别
- Level必须与Task::apiLevel()返回值一致
- Version第一位建议与Level相同

### 5. 依赖声明

```json
{
  "dependencies": [
    {
      "packageId": "cmn-official",
      "moduleId": "splitter-cmn",
      "level": 1,              // Level要求（用于兼容性检查）
      "version": ">=1.0.0"     // Version要求（用于Bug修复检查）
    }
  ]
}
```

## API参考

### LevelCompatibilityChecker

```cpp
class LevelCompatibilityChecker {
public:
    /// Level 兼容性检查结果
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
    int getRecommendedPluginLevel() const;

    // 检查Plugin是否需要适配器
    bool needsAdapter(int pluginLevel) const;

    // 生成兼容性报告
    std::string generateCompatibilityReport(
        const std::vector<LevelCompatibilityResult>& results
    ) const;
};
```

### LevelCompatibilityResult

```cpp
struct LevelCompatibilityResult {
    bool isCompatible;                          // 是否兼容
    int managerLevel;                           // Manager的Level
    int pluginLevel;                            // 插件的Level
    PluginCategory category;                     // 插件类别
    LevelCompatibilityError errorCode;           // 错误代码
    std::string pluginId;                       // 插件ID
    std::string moduleId;                       // 模块ID
    std::string message;                        // 详细消息
    std::string suggestion;                     // 建议操作
    std::vector<std::string> affectedModules;   // 受影响的模块

    // 获取格式化的错误报告
    std::string getFormattedReport() const;
};
```

### LevelManager

```cpp
class LevelManager {
public:
    // 获取当前系统的Manager Level
    static int getCurrentManagerLevel();

    // 检查Level是否有效
    static bool isValidLevel(int level);

    // 获取支持的Plugin Level范围
    static std::pair<int, int> getSupportedPluginLevelRange();

    // 检查插件类别是否需要Level检查
    static bool requiresLevelCheck(PluginCategory category);

    // 获取插件类别的名称
    static std::string getCategoryName(PluginCategory category);
};
```

### Task

```cpp
class Task {
public:
    Task();
    explicit Task(const ModuleSpec* spec);
    ~Task();

    // 返回插件的Level（必须实现）
    virtual int apiLevel() const = 0;

    // 初始化接口
    virtual Expected<void> initialize() = 0;

    // 执行接口
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput>& input) = 0;
};
```

## 最佳实践

### 1. Level管理

✅ **推荐做法**：

```cpp
// 明确声明Level
int apiLevel() const override {
    return 1;  // Level 1
}
```

```json
{
  "level": 1
}
```

❌ **不推荐做法**：

```cpp
// 不要使用负数或零
int apiLevel() const override {
    return 0;  // ❌ 无效的Level
}
```

### 2. Version命名

✅ **推荐做法**：

```json
{
  "level": 1,
  "version": "1.2.3"  // 第一位与Level相同
}
```

⚠️ **不推荐做法（但不影响兼容性）**：

```json
{
  "level": 2,
  "version": "1.5.0"  // 第一位与Level不同，但仍然兼容
}
```

### 3. 依赖声明

```json
{
  "dependencies": [
    {
      "packageId": "base-plugin",
      "moduleId": "splitter-base",
      "level": 1,              // Level要求（用于兼容性检查）
      "version": ">=1.0.0"     // Version要求（用于Bug修复检查）
    }
  ]
}
```

**重要**：
- `level`用于API兼容性检查
- `version`用于Bug修复要求检查
- 两者独立，互不影响

### 4. 错误处理

```cpp
Expected<NO<TaskResult>> start(const NO<TaskInput>& input) override {
    try {
        // 执行逻辑
        if (/* 错误条件 */) {
            return Error(Error::InvalidArgument, "Invalid input data");
        }

        // 成功
        return result;
    } catch (const std::exception& e) {
        return Error(Error::RuntimeError, e.what());
    }
}
```

## 故障排除

### 1. Level不兼容错误

**错误信息**:
```
Plugin Level 3 is higher than Manager Level 2
```

**解决方案**:
1. 升级Manager到Level 3
2. 或降级插件到Level 1或2

### 2. Level过低错误

**错误信息**:
```
Plugin Level 0 is too old for Manager Level 2 (requires Level >= 1)
```

**解决方案**:
1. 升级插件到Level 1或2
2. 或降级Manager到Level 1

### 3. 依赖Level冲突

**错误信息**:
```
Dependency splitter-cmn has Level incompatibility
```

**解决方案**:
1. 检查依赖的Level要求
2. 更新依赖以匹配Level要求
3. 使用诊断工具查看详细信息

### 4. 使用诊断工具

```cpp
// 定义系统Level配置
int currentLevel = 2;
int maximumLevel = 2;
int minimumLevel = 1;

// 运行诊断
auto result = LevelCompatibilityChecker::checkCorePlugin(
    1,              // pluginLevel
    currentLevel,
    maximumLevel,
    minimumLevel
);

// 生成详细报告
std::cout << result.message;

// 检查多个插件
auto results = LevelCompatibilityChecker::checkAll(
    corePlugins,
    dependencyPlugins,
    currentLevel,
    maximumLevel,
    minimumLevel
);
std::cout << LevelCompatibilityChecker::generateReport(results);
```

## 示例项目

### 完整的插件开发示例

1. **项目结构**:
```
my-plugin/
├── CMakeLists.txt
├── plugin.json
├── config.json
├── include/
│   └── MyPlugin.h
├── src/
│   └── MyPlugin.cpp
└── assets/
    └── dict/
```

2. **CMakeLists.txt**:
```cmake
cmake_minimum_required(VERSION 3.19)
project(MyPlugin)

find_package(LangCore REQUIRED)

add_library(MyPlugin SHARED src/MyPlugin.cpp)
target_link_libraries(MyPlugin LangCore::LangCore)
```

3. **编译和部署**:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

将生成的库文件和配置文件复制到LangCore的插件目录。

## 总结

Language Manager提供了基于Level的兼容性管理系统。通过本指南，您应该能够：

1. 理解Level兼容性规则
2. 开发兼容的插件
3. 使用Level诊断工具解决问题
4. 遵循最佳实践

**核心原则**：
- Level是API兼容性的唯一标准
- Version仅用于Bug修复，不影响兼容性
- 核心插件向下兼容一代
- 依赖插件不限制Level

如有更多问题，请参考技术架构分析文档或联系开发团队。