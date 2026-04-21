# Language Manager 插件开发指南

> 核心概念、接口定义和命名规范见 [PRD-v2.0.md](PRD-v2.0.md)。本文档仅覆盖实操流程。

**版本**：2.0  
**日期**：2026-04-21

---

## 1. 环境要求

- C++17, CMake 3.19+, MSVC 2019+ / GCC 9+ / Clang 11+
- vcpkg 依赖：`stdcorelib`, `nlohmann-json`

---

## 2. 项目结构

```
my-plugin/
├── CMakeLists.txt
├── MyPlugin.h / .cpp      # 插件类（继承 TaskPlugin 或 DriverPlugin）
├── MyTask.h / .cpp         # 任务类（继承 Task）
└── internal/V1/TaskImpl.h  # 版本化实现（可选）
```

**CMakeLists.txt**：

```cmake
cmake_minimum_required(VERSION 3.19)
project(MyPlugin LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(LangCore REQUIRED)

add_library(myPlugin SHARED MyPlugin.cpp MyTask.cpp)
target_link_libraries(myPlugin PRIVATE LangCore::LangCore)
install(TARGETS myPlugin LIBRARY DESTINATION plugins)
```

---

## 3. 最小示例

### TaskPlugin

```cpp
// MyPlugin.cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MyTask.h"

namespace LangPlugins::MyPlugin {

class MyPlugin final : public LangCore::TaskPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "g2p.my-custom"; }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    createTask(const LangCore::ModuleSpec *spec) override {
        return LangCore::NO<MyTask>::create(spec);
    }
};

} // namespace LangPlugins::MyPlugin

LANGCORE_EXPORT_PLUGIN(LangPlugins::MyPlugin::MyPlugin)
```

### Task

```cpp
// MyTask.h
#pragma once
#include <LangCore/Task/Task.h>

namespace LangPlugins::MyPlugin {

class MyTask : public LangCore::Task {
public:
    explicit MyTask(const LangCore::ModuleSpec *spec) : Task(spec) {}

    int apiLevel() const override { return 1; }

    LangCore::Expected<void> initialize() override {
        auto cfg = LangCore::config(spec());
        auto threshold = cfg.getDouble("threshold", 0.5);
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    start(const LangCore::NO<LangCore::TaskInput> &input) override {
        auto result = LangCore::NO<LangCore::TaskResult>::create();
        // 处理逻辑
        return result;
    }
};

} // namespace LangPlugins::MyPlugin
```

### DriverPlugin（AI 推理驱动）

```cpp
class MyDriverPlugin final : public LangCore::DriverPlugin {
public:
    int apiLevel() const override { return 1; }
    const char *key() const override { return "my-driver"; }

    LangCore::Expected<LangCore::NO<LangCore::SessionFactory>> create() override {
        return LangCore::NO<MySessionFactory>::create();
    }
};

LANGCORE_EXPORT_PLUGIN(MyDriverPlugin)
```

---

## 4. 多版本支持

当 Core API Level 升级时，使用 `VersionedTaskManager` 同时支持新旧版本：

```cpp
class MyTask : public LangCore::Task {
    LangCore::VersionedTaskManager<MyTask> _manager;

public:
    explicit MyTask(const LangCore::ModuleSpec *spec) : Task(spec), _manager(spec) {
        _manager.setImpl(std::make_unique<V1::TaskImpl>());
        // Level 2 时添加: _manager.setImpl(std::make_unique<V2::TaskImpl>());
    }

    int apiLevel() const override { return _manager.currentLevel(); }
    LangCore::Expected<void> initialize() override { return _manager.initialize(); }
    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    start(const LangCore::NO<LangCore::TaskInput> &input) override {
        return _manager.start(input);
    }
};
```

---

## 5. 打包

Package 格式为 `.lmpk`（ZIP），包含 `package.json`：

```json
{
  "packageId": "my-plugin",
  "version": "1.0.0",
  "vendor": "My Company",
  "modules": {
    "g2p": [
      {
        "moduleId": "g2p-my-custom",
        "class": "g2p.my-custom",
        "configuration": "config.json",
        "dependencies": []
      }
    ]
  }
}
```

详细格式说明见 PRD-v2.0.md 第 4 节。

---

## 6. 测试

```cpp
TEST(MyTaskIntegration, FullWorkflow) {
    auto mgr = LangCore::Manager::instance();
    std::string errMsg;
    ASSERT_TRUE(mgr->initialize(errMsg));

    auto task = mgr->task("g2p", "my-custom");
    ASSERT_TRUE(task.ok());

    auto result = task.get()->start(input);
    EXPECT_TRUE(result.ok());
}
```

---

## 7. 常见问题

**插件加载失败？** 检查：
1. 是否使用了 `LANGCORE_EXPORT_PLUGIN` 宏
2. 插件 `key()` 是否与 `package.json` 中的 `class` 字段匹配
3. 依赖是否已安装
4. 查看日志输出

**如何确定 Level？** Level 跟随 Core API 结构版本。使用了新版 Core 结构体就需要升级 Level。

---

**文档版本**: 2.0  
**最后更新**: 2026-04-21
