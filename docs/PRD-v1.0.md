# Language Manager 产品需求文档

## 文档说明

本文档定义 Language Manager 的核心设计规范和开发指南。

**核心目标**：实现长期免维护的第三方插件加载系统，遵循"Write Once, Run Forever"的设计理念。

**版本**：2.2
**日期**：2026-04-04

---

## 1. 产品概述

Language Manager 是一个基于 C++ 的可扩展语言处理框架，主要用于文本到语音转换（G2p）。

**核心特性**：
- **插件化架构**：所有功能模块作为插件动态加载
- **Level 兼容性系统**：Level 是 API 兼容性的唯一标准
- **包管理系统**：支持插件包的发现、依赖解析和加载
- **配置持久化**：支持用户配置的持久化存储和恢复
- **长期稳定性**：插件加载后常驻内存，无需复杂生命周期管理

**设计原则**：
- **简洁可靠**：允许必要时直接报错，记录日志，不设计重试或回滚机制
- **插件分类**：核心插件（使用 Core 结构体）和工具插件（独立功能）
- **统一规则**：Level 作为 API 兼容性的唯一标准
- **配置驱动**：通过配置文件灵活控制行为，无需重新编译

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
- **配置持久化**：自动保存用户配置到用户配置目录

**SessionTask**：AI 模型驱动的特殊任务
- `open(path, args)`：打开会话
- `close()`：关闭会话
- `isOpen()`：检查会话状态
- `id()`：获取会话 ID

---

## 3. 兼容性设计

### 3.1 Level 兼容性规则

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

### 3.2 LevelCompatibilityChecker

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

### 3.3 依赖解析

**DependencyResolver**：解析模块依赖关系，计算初始化顺序
**DependencyGraph**：构建依赖关系图，检测循环依赖

---

## 4. 错误处理规范

### 4.1 Error 类

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

### 4.2 Expected<T> 类型

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

## 5. 最佳实践

### 5.1 错误处理
1. **遇到错误时直接返回 `Expected<T>` 错误**
2. **不设计重试机制**
3. **不设计回滚机制**
4. **错误信息应该清晰、具体**
5. **使用 Logger 记录关键操作和错误信息**

### 5.2 配置管理
1. **使用 JSON 格式定义配置**
2. **推荐使用 ConfigAccessor**：`auto cfg = LangCore::config(spec())`
3. **必需字段使用 `Expected<T>` 返回类型的方法**（如 `getString()`）
4. **可选字段使用带默认值的方法**（如 `getString(key, defaultValue)`）
5. **支持配置热更新（通过 `setConfig`）**
6. **配置验证由 Task 内部负责**

### 5.3 插件开发
1. **插件加载后常驻内存，无需复杂生命周期管理**
2. **避免不必要的抽象层**
3. **优先保证代码简洁性**
4. **正确实现 `apiLevel()` 方法**
5. **使用兼容性检查器验证插件兼容性**

### 5.4 日志记录
1. **使用现有的日志系统**
2. **记录关键操作和错误信息**
3. **日志级别**：Trace、Debug、Info、Success、Warning、Critical、Fatal

---

## 6. 相关文档

- **API 使用指南**：`docs/API-Usage-Guide.md` - Level 兼容性管理
- **插件开发指南**：`docs/Plugin-Development-Guide.md` - 插件开发完整指南
- **核心架构**：`docs/Core-Architecture.md` - 核心架构设计
- **ChainG2p 设计**：`docs/ChainG2p-Design-Document.md` - ChainG2p 责任链架构

---

**文档版本**: 2.2
**最后更新**: 2026-04-04
**更新内容**:
- 精简文档结构，去除冗余内容
- 专注于核心需求和设计原则
- 优化最佳实践章节