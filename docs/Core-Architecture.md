# Language Manager 核心架构文档

## 文档说明

本文档描述 Language Manager 的核心架构设计，包括主要组件、它们之间的关系以及设计决策。

**版本**：1.1
**日期**：2026-04-04
**维护者**：Language Manager 开发团队

---

## 目录

1. [架构概述](#架构概述)
2. [核心组件](#核心组件)
3. [组件关系](#组件关系)
4. [设计模式](#设计模式)
5. [数据流](#数据流)
6. [扩展点](#扩展点)

---

## 架构概述

Language Manager 采用分层插件化架构，核心提供基础服务，所有功能通过插件实现。

### 架构层次

```
┌─────────────────────────────────────────┐
│         应用层 (Application)            │
│  (文本处理、语言转换、推理应用等)        │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      任务层 (Task Layer)                │
│  (Task、SessionTask、TaskPlugin)        │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      管理层 (Management Layer)          │
│  (Manager、PackageManager、PluginFactory)│
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      支持层 (Support Layer)             │
│  (Error、Expected、ConfigAccessor等)    │
└─────────────────────────────────────────┘
```

### 核心设计原则

1. **插件优先**：所有功能都通过插件实现，核心只提供基础服务
2. **依赖注入**：通过 ModuleSpec 和 PluginFactory 实现松耦合
3. **类型安全**：使用强类型接口，编译时检查错误
4. **错误处理**：统一使用 Expected<T> 进行错误处理，避免异常
5. **配置驱动**：通过 JSON 配置文件控制行为，无需重新编译

---

## 核心组件

### 1. Manager

**职责**：管理器单例，提供高层 API 和插件协调

**关键特性**：
- 单例模式，全局唯一实例
- 继承自 PackageManager，管理所有插件和包
- 提供便捷的文本处理 API（split、tag、convert）
- 自动加载和管理插件生命周期

**主要接口**：
```cpp
class Manager : public PackageManager {
public:
    static Manager *instance();
    bool initialize(std::string &errMsg);
    bool initialized() const;

    // 任务管理
    Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
    Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

    // 高级 API
    std::vector<std::string> split(const std::string &input);
    std::vector<TaggerRes> tag(const std::vector<std::string> &input, ...);
    std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);
};
```

### 2. PackageManager

**职责**：管理插件包、模块和插件发现

**关键特性**：
- 扫描和管理插件包（.lmpk 格式）
- 解析依赖关系，计算加载顺序
- 创建和管理插件实例
- 提供模块分类注册机制

**主要接口**：
```cpp
class PackageManager : public PluginFactory {
public:
    // 包管理
    void addPackagePath(const std::filesystem::path &path);
    Expected<Package> open(const std::filesystem::path &path);
    Package find(const std::string_view &id, const stdc::VersionNumber &version) const;

    // 模块管理
    ModuleCategory *category(const std::string_view &name) const;
    Expected<NO<Task>> createModuleTask(const ModuleMetadata &moduleInfo, const Package &pkg) const;

    // 依赖管理
    bool checkDependencies();
    std::vector<PackageInitializationPlan> getPackageInitializationOrder();
};
```

### 3. PluginFactory

**职责**：插件工厂，负责插件的加载和实例化

**关键特性**：
- 动态加载插件动态库
- 管理插件生命周期
- 提供插件注册机制

**主要接口**：
```cpp
class PluginFactory {
public:
    Expected<void> loadPlugin(const std::filesystem::path &path);
    Expected<NO<Plugin>> getPlugin(const std::string &iid, const std::string &key) const;
};
```

### 4. Task

**职责**：任务基类，定义统一的任务接口

**关键特性**：
- 提供统一的 API 接口
- 支持配置管理
- 支持多版本（通过 apiLevel()）

**主要接口**：
```cpp
class Task : public NamedObject {
public:
    virtual int apiLevel() const = 0;
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

    // 配置 API
    virtual std::string getConfig() const;
    virtual Expected<void> setConfig(const std::string &config);
    virtual std::string getUiSchema() const;
    virtual Expected<void> resetToDefault();
    virtual bool isUsingDefaultConfig() const;
};
```

### 5. SessionTask

**职责**：AI 模型驱动的特殊任务，支持会话管理

**关键特性**：
- 支持模型文件加载
- 会话生命周期管理
- 资源管理

**主要接口**：
```cpp
class SessionTask : public Task {
public:
    virtual Expected<void> open(const std::filesystem::path &path,
                               const NO<TaskInitArgs> &args) = 0;
    virtual Expected<void> close() = 0;
    virtual bool isOpen() const = 0;
    virtual int64_t id() const = 0;
};
```

### 6. ModuleSpec

**职责**：模块规范，描述模块的元数据和配置

**关键特性**：
- 存储模块元数据
- 提供配置访问
- 支持状态管理

**主要接口**：
```cpp
class ModuleSpec {
public:
    const std::string &id() const;
    const std::string &category() const;
    int apiLevel() const;
    const JsonObject &manifestConfiguration() const;
    NO<TaskConfiguration> configuration() const;
    const std::filesystem::path &path() const;
    State state() const;
    Package parent() const;
};
```

### 7. ModuleCategory

**职责**：模块分类，管理同类型的模块

**关键特性**：
- 模块发现和注册
- 模块解析和加载
- ObjectPool 模式，管理模块实例

**主要接口**：
```cpp
class ModuleCategory : public ObjectPool {
public:
    const std::string &name() const;
    std::vector<ModuleSpec *> findSpec(const ModuleLocator &identifier) const;
    std::vector<ModuleSpec *> specs() const;
};
```

### 8. Package

**职责**：插件包，封装插件包的元数据和资源

**关键特性**：
- 包元数据管理
- 资源访问
- 模块管理

**主要接口**：
```cpp
class Package {
public:
    const std::string &id() const;
    stdc::VersionNumber version() const;
    std::vector<PackageMetadata> dependencies() const;
    std::filesystem::path path() const;
};
```

---

## 组件关系

### 组件依赖图

```
Manager (单例)
    ↓ 继承
PackageManager
    ↓ 继承
PluginFactory
    ↓ 使用
Plugin ← TaskPlugin, DriverPlugin
    ↓ 创建
Task ← SessionTask
    ↓ 使用
ModuleSpec
    ↓ 属于
ModuleCategory
    ↓ 属于
Package
    ↓ 使用
Expected<T>, Error, ConfigAccessor
```

### 关键关系

1. **Manager → PackageManager**：Manager 继承 PackageManager，扩展高层 API
2. **PackageManager → PluginFactory**：PackageManager 继承 PluginFactory，增加包管理功能
3. **PluginFactory → Plugin**：PluginFactory 负责加载和管理 Plugin 实例
4. **TaskPlugin → Task**：TaskPlugin 创建 Task 实例
5. **DriverPlugin → SessionFactory**：DriverPlugin 创建 SessionFactory 实例
6. **SessionFactory → SessionTask**：SessionFactory 创建 SessionTask 实例
7. **ModuleSpec → ModuleCategory**：ModuleSpec 属于 ModuleCategory
8. **ModuleCategory → PackageManager**：ModuleCategory 由 PackageManager 管理
9. **ModuleSpec → Package**：ModuleSpec 属于 Package

---

## 设计模式

### 1. 单例模式（Singleton）

**应用**：Manager

**目的**：确保全局只有一个 Manager 实例

**实现**：
```cpp
class Manager {
public:
    static Manager *instance();
private:
    Manager();
    ~Manager();
};
```

### 2. 工厂模式（Factory）

**应用**：PluginFactory, TaskPlugin, DriverPlugin

**目的**：封装对象创建逻辑，支持动态扩展

**实现**：
```cpp
class TaskPlugin {
public:
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};

class DriverPlugin {
public:
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

### 3. 对象池模式（Object Pool）

**应用**：ModuleCategory

**目的**：管理模块实例的生命周期，支持复用

**实现**：
```cpp
class ModuleCategory : public ObjectPool {
public:
    template <class T>
    Expected<NO<T>> acquire(const ModuleLocator &loc);
    void release(NamedObject *obj);
};
```

### 4. 策略模式（Strategy）

**应用**：Task 的多版本实现

**目的**：支持同一任务的不同实现策略

**实现**：
```cpp
class VersionedTaskImplBase {
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;
};

class V1::TaskImpl : public VersionedTaskImplBase { ... };
class V2::TaskImpl : public VersionedTaskImplBase { ... };
```

### 5. 依赖注入（Dependency Injection）

**应用**：ModuleSpec, PluginFactory

**目的**：松耦合，支持测试和替换

**实现**：
```cpp
class Task {
public:
    explicit Task(const ModuleSpec *spec);
    PackageManager *Mgr() const;
    Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;
};
```

---

## 数据流

### 1. 初始化流程

```
Manager::initialize()
    ↓
PackageManager::loadPackagesInOrder()
    ↓
PackageManager::scanPackageDirectory()
    ↓
PackageManager::processPackageJson()
    ↓
PackageManager::collectModuleMetadata()
    ↓
ModuleCategory::parseSpec()
    ↓
PluginFactory::loadPlugin()
    ↓
TaskPlugin::createTask()
    ↓
Task::initialize()
```

### 2. 任务执行流程

```
Manager::task(category, id)
    ↓
PackageManager::createModuleTask()
    ↓
TaskPlugin::createTask()
    ↓
Task::initialize()
    ↓
Task::start(input)
    ↓
TaskResult
```

### 3. 配置管理流程

```
Task::getConfig()
    ↓
Task::loadConfig()
    ↓
优先加载用户配置，回退到默认配置
    ↓
返回配置字符串

Task::setConfig(config)
    ↓
Task::saveConfig(config)
    ↓
保存到用户配置目录
```

---

## 扩展点

### 1. 插件扩展

**扩展点**：PluginFactory

**扩展方式**：实现 Plugin 接口并注册

**示例**：
```cpp
class MyPlugin : public TaskPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "my-plugin"; }
    Expected<NO<Task>> createTask(const ModuleSpec *spec) override;
};

LANGCORE_EXPORT_PLUGIN(MyPlugin)
```

### 2. 模块分类扩展

**扩展点**：ModuleCategory

**扩展方式**：使用 LANGCORE_DECLARE_MODULE_CATEGORY 和 LANGCORE_DEFINE_MODULE_CATEGORY 宏

**示例**：
```cpp
LANGCORE_DECLARE_MODULE_CATEGORY(My, "my-category")
LANGCORE_DEFINE_MODULE_CATEGORY(My, "my-category")
```

### 3. 任务扩展

**扩展点**：Task

**扩展方式**：继承 Task 并实现虚函数

**示例**：
```cpp
class MyTask : public Task {
public:
    explicit MyTask(const ModuleSpec *spec);
    int apiLevel() const override;
    Expected<void> initialize() override;
    Expected<NO<TaskResult>> start(const NO<TaskInput> &input) override;
};
```

### 4. 配置扩展

**扩展点**：Task::getConfig() 和 Task::setConfig()

**扩展方式**：在子类中重写配置方法

**示例**：
```cpp
class MyTask : public Task {
public:
    std::string getConfig() const override {
        // 返回自定义配置
    }

    Expected<void> setConfig(const std::string &config) override {
        // 处理自定义配置
    }
};
```

---

**文档版本**: 1.1
**最后更新**: 2026-04-04
**更新内容**:
- 精简文档结构，去除冗余内容
- 保留核心组件和关键接口
- 优化组件关系和数据流说明
- 简化设计模式说明