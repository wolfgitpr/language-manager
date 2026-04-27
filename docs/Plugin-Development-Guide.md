# Language Manager 插件开发指南

> 核心概念、接口定义和命名规范见 [PRD-v2.0.md](PRD-v2.0.md)。本文档仅覆盖实操流程。

**版本**：4.0  
**日期**：2026-04-27

---

## 1. 环境要求

- C++17, CMake 3.19+, MSVC 2019+ / GCC 9+ / Clang 11+
- vcpkg 依赖：`stdcorelib`, `nlohmann-json`

---

## 2. 项目结构

```
my-plugin/
├── CMakeLists.txt
├── main.cpp                # 插件导出（使用简化宏）
├── MyTask.h / .cpp         # 任务类（继承 Task）
└── internal/V1/TaskImpl.h  # 版本化实现（可选）
```

**CMakeLists.txt**：

```cmake
cmake_minimum_required(VERSION 3.19)
project(MyPlugin LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(LangCore REQUIRED)

add_library(myPlugin SHARED main.cpp MyTask.cpp)
target_link_libraries(myPlugin PRIVATE LangCore::LangCore)
install(TARGETS myPlugin LIBRARY DESTINATION plugins)
```

---

## 3. 最小示例

### 使用简化宏（推荐）

```cpp
// main.cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MyTask.h"

using namespace LangCore;
using namespace LangPlugins::MyPlugin;

LANGCORE_DEFINE_TASK_PLUGIN(
    MyPlugin,      // 插件类名（宏自动生成）
    MyTask,        // 任务类名
    "g2p.my-custom", // 插件 key（须与 package.json 的 class 匹配）
    1              // API Level
)
```

### Task

```cpp
// MyTask.h
#pragma once
#include <LangCore/Task/Task.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>

namespace LangPlugins::MyPlugin {

    LangCore::LogCategory Log("myPlugin");

    class MyTask : public LangCore::Task {
    public:
        explicit MyTask(const LangCore::ModuleSpec *spec) : Task(spec) {}

        int apiLevel() const override { return 1; }

        LangCore::Expected<void> initialize() override {
            auto cfg = LangCore::config(spec());
            auto threshold = cfg.getDouble("threshold", 0.5);
            Log.langCoreInfo("Initialized with threshold: %1", threshold);
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

### 调用 Task（含 Context）

`Manager::initialize()` 返回 `Expected<void>`，而非 `bool`。`Manager::task()` 需要传入 `context` 参数来指定上下文：

```cpp
#include <LangCore/Core/Manager.h>

auto mgr = LangCore::Manager::instance();

// initialize() 返回 Expected<void>
auto initResult = mgr->initialize();
if (!initResult) {
    // 处理错误
    return;
}

// task() 的第二个参数是 context（字符串）
// 空字符串 "" 表示默认 context
auto taskResult = mgr->task("g2p", "", "g2p-my-custom");

// 指定 context + version
auto taskResult2 = mgr->task("g2p", "SingerA",
                              stdc::VersionNumber(2, 0, 0), "g2p-cmn");
```

### DriverPlugin（AI 推理驱动）

```cpp
// main.cpp
#include <LangCore/Task/TaskPlugin.h>
#include "MySessionFactory.h"

using namespace LangCore;

LANGCORE_DEFINE_DRIVER_PLUGIN(
    MyDriverPlugin,
    MySessionFactory,
    "my-driver",
    1
)
```

---

## 4. Context 系统与插件的关系

Context 系统用于支持多角色（如不同歌手）使用不同的包和配置。**插件本身无需感知 Context**——框架自动完成路由。

### 工作原理

1. 前端调用 `Manager::task("g2p", "SingerA", "g2p-cmn")` 时指定 context
2. 框架根据 context 查找对应的 Package 和模块配置
3. 插件通过 `spec()` 获取的 `ModuleSpec` 已经绑定到正确的 context
4. 插件的 `initialize()` / `start()` 代码**不需要任何修改**即可支持多 context

### 对插件开发者的影响

- **无需关心 context 参数**：`Task` 接口没有 context 相关方法
- **配置自动隔离**：不同 context 下同一插件的 `config.json` 可以不同
- **透明路由**：同一插件类可以在多个 context 中各自实例化，互不干扰

---

## 5. 多版本支持

当 Core API Level 升级时，使用 `VersionedTaskManager` + `TASK_IMPLEMENT` 宏同时支持新旧版本：

```cpp
// MyTask.h
class MyTask : public LangCore::Task {
public:
    explicit MyTask(const LangCore::ModuleSpec *spec);
    ~MyTask() override;

    int apiLevel() const override;
    LangCore::Expected<void> initialize() override;
    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    start(const LangCore::NO<LangCore::TaskInput> &input) override;
    std::string getConfig() const override;

private:
    LangCore::VersionedTaskManager _manager;
};
```

**单版本插件（推荐）**：

```cpp
// MyTask.cpp
#include "MyTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::MyPlugin {
    TASK_IMPLEMENT(MyTask, Internal::V1::MyTaskImpl)
}
```

`TASK_IMPLEMENT(TaskClass, ImplClass)` 自动生成构造函数和所有委托方法。

**多版本插件**：手动编写构造函数，使用 `TASK_IMPLEMENT_METHODS` 生成其余方法：

```cpp
// MyTask.cpp
MyTask::MyTask(const LangCore::ModuleSpec *spec) : Task(spec), _manager(spec) {
    switch (spec->apiLevel()) {
        case 2: _manager.setImpl(std::make_unique<Internal::V2::MyTaskImpl>(spec)); break;
        default: _manager.setImpl(std::make_unique<Internal::V1::MyTaskImpl>(spec)); break;
    }
}
TASK_IMPLEMENT_METHODS(MyTask)
```

---

## 6. 错误处理

使用 `Expected<T>` 传播错误，不抛出异常：

```cpp
LangCore::Expected<void> initialize() override {
    auto cfg = LangCore::config(spec());

    // 必需字段——缺失时自动返回 ConfigError
    auto modelPath = cfg.getPath("model_path");
    if (!modelPath) return modelPath.takeError();

    // 可选字段——带默认值
    auto batchSize = cfg.getInt("batch_size", 50);

    return {};
}
```

仅在调用第三方库时使用 try-catch，将异常转为 `Error`（详见 PRD §5.4）。

---

## 7. 打包

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

详细格式说明见 PRD §4。

---

## 8. 常见问题

**插件加载失败？** 检查：
1. 是否使用了 `LANGCORE_EXPORT_PLUGIN` 宏（或使用 `LANGCORE_DEFINE_TASK_PLUGIN` 简化宏）
2. 插件 `key()` 是否与 `package.json` 中的 `class` 字段匹配
3. 依赖是否已安装
4. 查看日志输出（`MgrLog` / `PluginLog` 分类）

**如何确定 Level？** Level 跟随 Core API 结构版本。当 `TaskInput` / `TaskResult` 等结构体布局变化时递增 Level。同一插件可通过 `VersionedTaskManager` 同时支持多个 Level。

---

**文档版本**: 4.0  
**最后更新**: 2026-04-27
