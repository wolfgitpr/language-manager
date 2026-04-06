# Language Manager 插件开发指南

## 文档说明

本文档提供 Language Manager 插件开发的完整指南，包括插件类型、开发流程、最佳实践和示例代码。

**版本**：1.1
**日期**：2026-04-04
**目标读者**：插件开发者、架构师、技术负责人

---

## 目录

1. [插件概述](#插件概述)
2. [插件类型](#插件类型)
3. [开发环境搭建](#开发环境搭建)
4. [插件开发流程](#插件开发流程)
5. [插件接口详解](#插件接口详解)
6. [配置管理](#配置管理)
7. [测试与调试](#测试与调试)
8. [打包与发布](#打包与发布)
9. [常见问题](#常见问题)
10. [最佳实践](#最佳实践)

---

## 插件概述

Language Manager 采用插件化架构，所有功能都通过插件实现。插件是独立编译的动态库，可以在运行时动态加载。

### 插件特点

- **动态加载**：运行时加载，无需重新编译核心
- **热插拔**：支持插件的加载和卸载
- **版本兼容**：通过 Level 机制保证版本兼容性
- **配置驱动**：通过配置文件控制行为
- **独立开发**：插件可以独立开发和测试

### 插件结构

```
my-plugin/
├── CMakeLists.txt          # 构建配置
├── main.cpp                # 插件入口
├── MyTask.h                # 任务头文件
├── MyTask.cpp              # 任务实现
├── config.json             # 默认配置
└── assets/                 # 资源文件
    ├── dict.txt
    └── model.onnx
```

---

## 插件类型

### 1. TaskPlugin（任务插件）

**用途**：实现具体的处理逻辑，如 G2p 转换、文本分割、语言标记等

**接口**：
```cpp
class TaskPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

**适用场景**：
- G2p 转换（g2p-cmn、g2p-eng）
- 文本分割（splitter-regex）
- 语言标记（tagger-template）

**示例**：
```cpp
class MyTaskPlugin : public TaskPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "g2p.my-custom"; }

    Expected<NO<Task>> createTask(const ModuleSpec *spec) override {
        return NO<MyTask>::create(spec);
    }
};

LANGCORE_EXPORT_PLUGIN(MyTaskPlugin)
```

### 2. DriverPlugin（驱动插件）

**用途**：提供 AI 模型推理能力，如 ONNX Runtime、TensorFlow、PyTorch 等

**接口**：
```cpp
class DriverPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

**适用场景**：
- ONNX Runtime 驱动
- TensorFlow 驱动
- PyTorch 驱动

**示例**：
```cpp
class OnnxDriverPlugin : public DriverPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "onnx"; }

    Expected<NO<SessionFactory>> create() override {
        return NO<OnnxSessionFactory>::create();
    }
};

LANGCORE_EXPORT_PLUGIN(OnnxDriverPlugin)
```

---

## 开发环境搭建

### 1. 环境要求

- **操作系统**：Windows 10/11、Linux、macOS
- **编译器**：MSVC 2019/2022、GCC 9+、Clang 11+
- **CMake**：3.19+
- **C++ 标准**：C++17

### 2. 依赖安装

```bash
# 安装 stdcorelib
vcpkg install stdcorelib

# 安装 nlohmann-json
vcpkg install nlohmann-json
```

### 3. 项目创建

```cmake
cmake_minimum_required(VERSION 3.19)
project(MyPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LangCore
find_package(LangCore REQUIRED)

# 添加插件
add_library(myPlugin SHARED
    main.cpp
    MyTask.cpp
)

target_link_libraries(myPlugin
    PRIVATE
    LangCore::LangCore
)

# 安装插件
install(TARGETS myPlugin
    LIBRARY DESTINATION plugins
)
```

---

## 插件开发流程

### 1. 定义插件类

```cpp
// MyTaskPlugin.h
#pragma once
#include <LangCore/Task/TaskPlugin.h>
#include "MyTask.h"

namespace LangPlugins::MyPlugin
{
    class MyTaskPlugin final : public LangCore::TaskPlugin {
    public:
        MyTaskPlugin() = default;

        int apiLevel() const override;
        const char *key() const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override;
    };
}
```

### 2. 实现插件类

```cpp
// MyTaskPlugin.cpp
#include "MyTaskPlugin.h"
#include <LangCore/LangCoreGlobal.h>

namespace LangPlugins::MyPlugin
{
    int MyTaskPlugin::apiLevel() const {
        return 1;
    }

    const char *MyTaskPlugin::key() const {
        return "g2p.my-custom";
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    MyTaskPlugin::createTask(const LangCore::ModuleSpec *spec) {
        return LangCore::NO<MyTask>::create(spec);
    }
}

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyPlugin::MyTaskPlugin)
```

### 3. 实现任务类

```cpp
// MyTask.h
#pragma once
#include <LangCore/Task/Task.h>

namespace LangPlugins::MyPlugin
{
    class MyTask : public LangCore::Task {
    public:
        explicit MyTask(const LangCore::ModuleSpec *spec);
        ~MyTask() override;

        int apiLevel() const override;
        LangCore::Expected<void> initialize() override;
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>> start(
            const LangCore::NO<LangCore::TaskInput> &input) override;
    };
}
```

```cpp
// MyTask.cpp
#include "MyTask.h"
#include <LangCore/Support/ConfigAccessor.h>

namespace LangPlugins::MyPlugin
{
    MyTask::MyTask(const LangCore::ModuleSpec *spec) : LangCore::Task(spec) {}

    MyTask::~MyTask() = default;

    int MyTask::apiLevel() const {
        return 1;
    }

    LangCore::Expected<void> MyTask::initialize() {
        auto cfg = LangCore::config(spec());

        // 获取配置
        auto enabled = cfg.getBool("enabled", true);
        auto threshold = cfg.getDouble("threshold", 0.5);

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    MyTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        auto result = LangCore::NO<LangCore::TaskResult>::create();
        // 处理逻辑
        return result;
    }
}
```

### 4. 创建插件入口

```cpp
// main.cpp
#include "MyTaskPlugin.h"

// 插件导出在 MyTaskPlugin.cpp 中使用 LANGCORE_EXPORT_PLUGIN 宏
```

---

## 插件接口详解

### Plugin 基类

```cpp
class Plugin {
public:
    virtual const char *iid() const = 0;  // 接口标识符
    virtual const char *key() const = 0;  // 插件键名
    virtual int apiLevel() const = 0;    // API 级别
};
```

### TaskPlugin 接口

```cpp
class TaskPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

### DriverPlugin 接口

```cpp
class DriverPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

### SessionFactory 接口

```cpp
class SessionFactory : public NamedObject {
public:
    virtual std::string arch() const = 0;
    virtual std::string backend() const = 0;
    virtual Expected<void> initialize(const NO<TaskInitArgs> &args) = 0;
    virtual NO<SessionTask> createSession() = 0;
};
```

### SessionTask 接口

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

---

## 配置管理

### ConfigAccessor 使用

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

### 配置 API

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

### 配置持久化

Task 基类提供自动配置持久化功能：

1. **配置加载**：
   - 优先加载用户配置（`~/.config/language-manager/[taskId]/config.json`）
   - 回退到默认配置（`assets/config.json`）

2. **配置保存**：
   - 调用 `setConfig()` 时自动保存到用户配置目录
   - 支持配置热更新

3. **配置重置**：
   - 调用 `resetToDefault()` 删除用户配置文件
   - 下次加载时使用默认配置

---

## 测试与调试

### 单元测试

```cpp
#include <LangCore/Task/Task.h>
#include <gtest/gtest.h>

TEST(MyTaskTest, Initialize) {
    // 创建测试配置
    std::string config = R"({"enabled": true})";

    // 创建测试实例
    auto task = createTestTask(config);

    // 测试初始化
    auto result = task->initialize();
    EXPECT_TRUE(result.ok());
}
```

### 集成测试

```cpp
TEST(MyTaskIntegration, FullWorkflow) {
    // 初始化 Manager
    auto mgr = LangCore::Manager::instance();
    std::string errMsg;
    ASSERT_TRUE(mgr->initialize(errMsg));

    // 获取任务
    auto task = mgr->task("g2p", "my-custom");
    ASSERT_TRUE(task.ok());

    // 执行任务
    auto result = task->start(input);
    EXPECT_TRUE(result.ok());
}
```

### 调试技巧

1. **使用日志**：
```cpp
#include <LangCore/Support/Logging.h>

LOG_INFO("Task initialized");
LOG_ERROR("Failed to load config: {}", error.message());
```

2. **使用 Expected 检查错误**：
```cpp
auto result = task->initialize();
if (!result) {
    auto err = result.takeError();
    std::cerr << "Error: " << err.message() << std::endl;
    if (err.hasSuggestion()) {
        std::cerr << "Suggestion: " << err.suggestion() << std::endl;
    }
}
```

---

## 打包与发布

### 包结构

```
my-plugin.lmpk (ZIP 格式)
├── package.json
├── modules/
│   └── my-category/
│       └── module.json
├── configs/
│   └── my-task.json
└── assets/
    ├── config.json
    ├── dict.txt
    └── model.onnx
```

### package.json

```json
{
  "packageId": "my-plugin",
  "version": "1.0.0",
  "vendor": "My Company",
  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-my-custom",
        "className": "MyTask",
        "apiLevel": 1,
        "version": "1.0.0"
      }
    ]
  }
}
```

### module.json

```json
{
  "id": "g2p-my-custom",
  "category": "g2p",
  "className": "MyTask",
  "apiLevel": 1,
  "version": "1.0.0",
  "enabled": true
}
```

---

## 常见问题

### Q1: 如何确定插件的 Level？

A: 插件的 Level 应该与 Core API 的结构版本一致。如果插件使用了新的 Core 结构，则需要升级 Level。

### Q2: 如何处理插件的依赖？

A: 在 package.json 中声明依赖：

```json
{
  "dependencies": [
    {
      "packageId": "cmn-official",
      "moduleId": "splitter-cmn",
      "level": 1,
      "version": "*"
    }
  ]
}
```

### Q3: 如何调试插件加载失败？

A: 检查以下几点：
1. 插件是否正确导出（LANGCORE_EXPORT_PLUGIN）
2. 插件 key 是否与 module.json 中的 id 匹配
3. 依赖是否正确安装
4. 查看日志文件了解详细错误信息

### Q4: 如何支持多版本插件？

A: 使用 VersionedTaskManager：

```cpp
template<typename TaskType>
class VersionedTaskManager {
public:
    explicit VersionedTaskManager(const ModuleSpec *spec);
    void setCurrentLevel(int level);
    void setImpl(std::unique_ptr<VersionedTaskImplBase> impl);
    Expected<void> initialize();
    Expected<NO<TaskResult>> start(const NO<TaskInput> &input);
};
```

---

## 最佳实践

### 1. 命名规范

- **插件类名**：`[Name]Plugin`
- **任务类名**：`[Name]Task`
- **命名空间**：`LangPlugins::[PluginName]`
- **插件 key**：`category.plugin-name`

### 2. 错误处理

- 始终使用 `Expected<T>` 返回结果
- 提供清晰的错误信息和改进建议
- 避免使用异常

### 3. 配置管理

- 使用 ConfigAccessor 进行类型安全的配置获取
- 必需字段使用 `Expected<T>` 返回类型
- 可选字段使用带默认值的方法

### 4. 性能优化

- 避免频繁的对象创建和销毁
- 使用对象池复用对象
- 考虑批量处理优化

### 5. 测试

- 编写单元测试覆盖核心逻辑
- 编写集成测试验证完整流程
- 使用 CI/CD 自动化测试

---

**文档版本**: 1.1
**最后更新**: 2026-04-04
**更新内容**:
- 精简文档结构，去除冗余内容
- 保留核心开发流程和关键信息
- 优化代码示例和最佳实践
- 简化测试和调试说明