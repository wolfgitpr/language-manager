# Task 引用计数系统 - 优化设计文档

## 版本信息

**版本**: 2.0
**创建日期**: 2026-04-05
**作者**: Language Manager Team
**状态**: 优化设计阶段

---

## 设计原则

### 核心原则

1. **简洁可靠**：最小化设计，避免过度工程
2. **职责单一**：每个组件只做一件事
3. **非侵入性**：不破坏现有架构
4. **延迟计算**：只在需要时计算引用计数
5. **数据驱动**：UI 基于数据，不依赖复杂逻辑

---

## 核心概念

### 1. 引用关系

**定义**：Task A 依赖 Task B，则 Task B 的引用计数 +1

**示例**：
```
tagger-eng → splitter-eng (refCount = 1)
g2p-chain-eng → splitter-eng (refCount = 2)
```

### 2. 引用计数规则

- **只统计直接依赖**：不递归统计
- **初始化成功才计数**：失败的依赖不计入
- **循环依赖不计入**：被拒绝加载
- **重复依赖去重**：同一模块多次声明只计一次

### 3. UI 显示规则

- **refCount > 1**：显示警告标签
- **警告内容**：显示引用者名称列表
- **无引用者**：不显示警告

---

## 简化架构

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                   PackageManager                              │
│  - 依赖解析（已有）                                         │
│  - 计算引用计数（新增）                                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  ModuleSpec                                  │
│  - refCount (新增)                                         │
│  - getReferenceCount() (新增)                              │
│  - getReferrers() (新增)                                   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                      Task                                   │
│  - getReferenceInfo() (新增，委托给 ModuleSpec)           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    UI Framework                             │
│  - 解析 UI Schema                                          │
│  - 显示引用警告（如适用）                                  │
└─────────────────────────────────────────────────────────────┘
```

### 关键改进

1. **去除 DependencyGraph 扩展**：引用计数计算在 PackageManager 层完成
2. **简化数据结构**：只存储必要信息（packageId, moduleId, name）
3. **去除复杂的 UI Schema 扩展**：使用简单的数据驱动方式
4. **去除 ReferenceInfo 结构**：直接使用 std::pair 或简单的结构体

---

## 数据结构（简化版）

### 1. ModuleSpec 扩展

```cpp
// core/lib/Module/Module_p.h

class ModuleSpec::Impl {
public:
    // 现有字段...
    int refCount = 0;                                      // 引用计数
    std::vector<std::pair<std::string, std::string>> referrers;  // 引用者列表 (packageId, moduleId)
};

// core/include/LangCore/Module/Module.h

class ModuleSpec {
public:
    // 获取引用计数
    int getReferenceCount() const;
    
    // 获取引用者列表
    std::vector<std::pair<std::string, std::string>> getReferrers() const;
    
    // 检查是否被多个 Task 引用
    bool isShared() const;
};
```

### 2. 简化的引用信息

```cpp
// core/lib/Module/Module.cpp

int ModuleSpec::getReferenceCount() const {
    return impl->refCount;
}

std::vector<std::pair<std::string, std::string>> ModuleSpec::getReferrers() const {
    return impl->referrers;
}

bool ModuleSpec::isShared() const {
    return impl->refCount > 1;
}
```

### 3. Task 委托

```cpp
// core/lib/Task/Task.cpp

int Task::getReferenceCount() const {
    if (!impl->spec) {
        return 0;
    }
    return impl->spec->getReferenceCount();
}

std::vector<std::pair<std::string, std::string>> Task::getReferrers() const {
    if (!impl->spec) {
        return {};
    }
    return impl->spec->getReferrers();
}

bool Task::isShared() const {
    if (!impl->spec) {
        return false;
    }
    return impl->spec->isShared();
}
```

---

## 引用计数计算（简化版）

### 计算时机

在 `PackageManager::resolveModuleDependencies()` 完成后立即计算

### 计算逻辑

```cpp
// core/lib/Core/PackageManager.cpp

void PackageManager::Impl::calculateReferenceCounts() {
    // 清空所有引用计数
    for (auto &moduleInfo : moduleInfos) {
        auto spec = findModuleSpec(moduleInfo.packageId, moduleInfo.moduleId);
        if (spec) {
            spec->impl->refCount = 0;
            spec->impl->referrers.clear();
        }
    }
    
    // 计算引用关系
    for (const auto &moduleInfo : moduleInfos) {
        auto spec = findModuleSpec(moduleInfo.packageId, moduleInfo.moduleId);
        if (!spec) continue;
        
        // 遍历该模块的所有依赖
        for (const auto &dep : moduleInfo.resolvedDependencies) {
            auto depSpec = findModuleSpec(dep.packageId, dep.moduleId);
            if (!depSpec) continue;
            
            // 增加引用计数
            depSpec->impl->refCount++;
            
            // 添加引用者
            depSpec->impl->referrers.emplace_back(
                moduleInfo.packageId,
                moduleInfo.moduleId
            );
        }
    }
}
```

### 调用位置

```cpp
// core/lib/Core/PackageManager.cpp

bool PackageManager::Impl::resolveModuleDependencies() {
    DependencyResolver resolver;
    
    // 解析依赖
    if (!resolver.resolveAllDependencies(moduleInfos)) {
        dependencyResolutionSuccessful = false;
        dependencyErrors = resolver.getErrors();
        return false;
    }
    
    moduleInfos = resolver.getResolvedModules();
    
    // 计算引用计数
    calculateReferenceCounts();
    
    return true;
}
```

---

## UI 集成（简化版）

### 1. 本地化文本

**位置**: `plugins/Task/assets/i18n.json`

```json
{
  "$version": "1.0",
  "pluginId": "Task",
  "reference": {
    "shared": {
      "warning": {
        "_": "This task is used by {count} tasks",
        "zh": "此任务被 {count} 个任务使用"
      },
      "referenced_by": {
        "_": "Referenced by:",
        "zh": "被以下任务引用："
      }
    }
  }
}
```

### 2. Task 扩展

```cpp
// core/include/LangCore/Task/Task.h

class Task {
public:
    // 获取引用计数
    int getReferenceCount() const;
    
    // 获取引用者列表
    std::vector<std::pair<std::string, std::string>> getReferrers() const;
    
    // 检查是否被共享
    bool isShared() const;
};
```

### 3. UI 显示逻辑

```cpp
// UI 框架实现

void TaskConfigWidget::renderReferenceInfo(Task *task) {
    if (!task->isShared()) {
        return;
    }
    
    int refCount = task->getReferenceCount();
    auto referrers = task->getReferrers();
    
    // 显示警告标题
    std::string warningKey = "reference.shared.warning";
    std::string warningText = PluginI18nLoader::getPluginText("Task", warningKey);
    
    // 替换占位符
    std::string warning = replacePlaceholder(warningText, "{count}", std::to_string(refCount));
    ui->showWarning(warning);
    
    // 显示引用者列表
    std::string refByKey = "reference.shared.referenced_by";
    std::string refByText = PluginI18nLoader::getPluginText("Task", refByKey);
    ui->showLabel(refByText);
    
    for (const auto &referrer : referrers) {
        auto name = getTaskName(referrer.first, referrer.second);
        ui->addReferrer(name);
    }
}

std::string TaskConfigWidget::getTaskName(
    const std::string &packageId, 
    const std::string &moduleId) {
    
    auto mgr = Manager::instance();
    auto spec = mgr->findModuleSpec(packageId, moduleId);
    
    if (!spec) {
        return packageId + "::" + moduleId;
    }
    
    // 返回本地化后的名称
    DisplayText name(spec->name());
    return name.text();
}
```

### 4. 占位符替换工具

```cpp
// core/lib/Support/StringUtils.h

namespace LangCore {

class StringUtils {
public:
    // 替换占位符
    static std::string replacePlaceholder(
        const std::string &text,
        const std::string &placeholder,
        const std::string &value);
    
    // 替换多个占位符
    static std::string replacePlaceholders(
        const std::string &text,
        const std::map<std::string, std::string> &placeholders);
};

} // namespace LangCore
```

---

## 边界情况处理（简化版）

### 1. 初始化失败

**处理方式**：初始化失败的 Task 不计入引用计数

**实现**：在 Task 创建时检查初始化结果

```cpp
Expected<NO<Task>> PackageManager::createModuleTask(
    const ModuleMetadata &moduleInfo,
    const Package &pkg) const {
    
    // 创建 Task
    auto taskExp = taskPlugin->createTask(moduleSpec);
    auto task = taskExp.take();
    
    // 初始化 Task
    auto initResult = task->initialize();
    if (!initResult) {
        // 初始化失败，不注册到 ObjectPool
        return initResult.takeError();
    }
    
    // 初始化成功，注册到 ObjectPool
    // 注意：引用计数已经在依赖解析时计算完成
    return task;
}
```

### 2. 循环依赖

**处理方式**：DependencyResolver 检测并拒绝循环依赖

**实现**：使用现有的循环依赖检测机制

```cpp
bool DependencyResolver::hasCycles() const {
    // 现有实现...
}
```

### 3. 包卸载

**处理方式**：不实时更新引用计数，下次重新加载时重新计算

**原因**：
- 简化实现
- 包卸载是低频操作
- 热重载时重新计算即可

### 4. 热重载

**处理方式**：重新计算引用计数

```cpp
void PackageManager::reloadPackage(const std::string &packageId) {
    // 卸载旧包
    unloadPackage(packageId);
    
    // 加载新包
    loadPackage(packageId);
    
    // 重新解析依赖
    resolveModuleDependencies();
    
    // 引用计数自动重新计算
}
```

---

## 完整示例（简化版）

### 1. 检查 Task 是否被共享

```cpp
#include <LangCore/Core/Manager.h>

auto mgr = LangCore::Manager::instance();
auto taskExp = mgr->task("splitter", "splitter-eng");

if (!taskExp) {
    return;
}

auto task = taskExp.value();

// 检查是否被共享
if (task->isShared()) {
    int refCount = task->getReferenceCount();
    auto referrers = task->getReferrers();
    
    std::cout << "⚠️  This task is shared by " << refCount << " tasks" << std::endl;
    
    for (const auto &referrer : referrers) {
        std::cout << "  • " << referrer.first << "::" << referrer.second << std::endl;
    }
}
```

### 2. UI 显示警告

```cpp
void TaskConfigWidget::renderReferenceInfo(Task *task) {
    if (!task->isShared()) {
        return;
    }
    
    // 获取引用计数和引用者
    int refCount = task->getReferenceCount();
    auto referrers = task->getReferrers();
    
    // 获取本地化文本
    std::string warningKey = "reference.shared.warning";
    std::string warningText = PluginI18nLoader::getPluginText("Task", warningKey);
    
    // 替换占位符
    std::string warning = replacePlaceholder(warningText, "{count}", std::to_string(refCount));
    
    // 显示警告
    ui->showWarning(warning);
    
    // 显示引用者列表
    for (const auto &referrer : referrers) {
        auto name = getTaskName(referrer.first, referrer.second);
        ui->addReferrer(name);
    }
}
```

### 3. 修改配置前确认

```cpp
Expected<void> updateTaskConfig(
    const std::string &category,
    const std::string &id,
    const std::string &newConfig)
{
    auto mgr = LangCore::Manager::instance();
    auto taskExp = mgr->task(category, id);
    
    if (!taskExp) {
        return taskExp.takeError();
    }
    
    auto task = taskExp.value();
    
    // 检查是否被共享
    if (task->isShared()) {
        int refCount = task->getReferenceCount();
        auto referrers = task->getReferrers();
        
        // 显示警告
        std::cout << "⚠️  This task is shared by " << refCount << " tasks" << std::endl;
        for (const auto &referrer : referrers) {
            auto name = getTaskName(referrer.first, referrer.second);
            std::cout << "  • " << name << std::endl;
        }
        
        // 确认修改
        if (!confirm("Continue?")) {
            return {};
        }
    }
    
    // 修改配置
    return task->setConfig(newConfig);
}
```

---

## 与本地化、UI方案的协同

### 1. 本地化集成

**简化的本地化结构**：
```json
{
  "reference": {
    "shared": {
      "warning": "This task is used by {count} tasks",
      "referenced_by": "Referenced by:"
    }
  }
}
```

**使用方式**：
```cpp
std::string warningText = PluginI18nLoader::getPluginText("Task", "reference.shared.warning");
std::string warning = replacePlaceholder(warningText, "{count}", std::to_string(refCount));
```

### 2. UI 模板化集成

**简化的 UI 显示**：
```cpp
void TaskConfigWidget::renderReferenceInfo(Task *task) {
    if (!task->isShared()) {
        return;
    }
    
    // 使用现有的 UI 组件
    ui->showWarning(getWarningText(task));
    ui->showReferrerList(getReferrerNames(task));
}
```

**不需要扩展 UI Schema**：
- 引用信息通过 Task API 直接获取
- UI 框架基于数据渲染
- 无需复杂的 UI Schema 扩展

---

## 实现检查清单

### 核心实现

- [ ] 在 `ModuleSpec::Impl` 中添加 `refCount` 和 `referrers`
- [ ] 在 `ModuleSpec` 中添加 `getReferenceCount()`, `getReferrers()`, `isShared()`
- [ ] 在 `Task` 中添加委托方法
- [ ] 在 `PackageManager::Impl` 中添加 `calculateReferenceCounts()`
- [ ] 在 `resolveModuleDependencies()` 中调用 `calculateReferenceCounts()`

### 工具函数

- [ ] 实现 `StringUtils::replacePlaceholder()`
- [ ] 实现 `StringUtils::replacePlaceholders()`

### 本地化

- [ ] 在 `plugins/Task/assets/i18n.json` 中添加本地化文本
- [ ] 更新现有本地化文件（如果需要）

### UI 集成

- [ ] 实现 `TaskConfigWidget::renderReferenceInfo()`
- [ ] 实现 `TaskConfigWidget::getTaskName()`
- [ ] 测试 UI 显示效果

### 测试

- [ ] 编写引用计数计算测试
- [ ] 编写边界情况测试
- [ ] 编写 UI 显示测试

---

## 与原方案的对比

### 改进点

| 方面 | 原方案 | 优化方案 |
|------|--------|----------|
| 数据结构 | 复杂的 `ReferenceInfo` 结构 | 简单的 `std::pair` |
| DependencyGraph | 扩展多个方法 | 不扩展，计算在 PackageManager |
| UI Schema | 复杂的扩展结构 | 不扩展，基于数据驱动 |
| 本地化 | 复杂的键名结构 | 简化的占位符方式 |
| 代码量 | 大量冗余代码 | 最小化实现 |
| 复杂度 | 高 | 低 |

### 去除的冗余

1. ❌ `ReferenceInfo` 结构（使用 `std::pair` 替代）
2. ❌ `DependencyGraph` 的扩展方法（计算在 PackageManager）
3. ❌ 复杂的 UI Schema 扩展（数据驱动）
4. ❌ 复杂的本地化键名结构（使用占位符）
5. ❌ 包卸载时的实时更新（延迟计算）

### 保留的核心功能

1. ✅ 引用计数计算
2. ✅ 引用者列表
3. ✅ UI 警告显示
4. ✅ 本地化支持
5. ✅ 边界情况处理

---

## 最佳实践

### 1. 性能优化

- **延迟计算**：只在依赖解析完成时计算一次
- **缓存结果**：引用计数存储在 ModuleSpec 中
- **避免重复计算**：不实时更新，只在需要时重新计算

### 2. 错误处理

- **日志记录**：记录引用计数计算错误
- **回退机制**：计算失败时返回 0
- **用户提示**：显示友好的错误信息

### 3. UI 设计

- **简洁明了**：警告信息简洁
- **数据驱动**：基于数据渲染，不依赖复杂逻辑
- **本地化支持**：所有文本可本地化

### 4. 数据一致性

- **原子更新**：引用计数和引用者列表原子更新
- **事务性**：依赖解析失败时回滚
- **验证机制**：定期验证引用计数

---

## 测试用例（简化版）

### 1. 基本引用计数

```cpp
TEST(ReferenceCountTest, Basic) {
    auto mgr = Manager::instance();
    auto taskExp = mgr->task("splitter", "splitter-eng");
    
    ASSERT_TRUE(taskExp.ok());
    auto task = taskExp.value();
    
    int refCount = task->getReferenceCount();
    EXPECT_GE(refCount, 0);
}
```

### 2. 共享检查

```cpp
TEST(ReferenceCountTest, IsShared) {
    auto mgr = Manager::instance();
    auto taskExp = mgr->task("splitter", "splitter-eng");
    
    ASSERT_TRUE(taskExp.ok());
    auto task = taskExp.value();
    
    bool isShared = task->isShared();
    int refCount = task->getReferenceCount();
    
    EXPECT_EQ(isShared, refCount > 1);
}
```

### 3. 引用者列表

```cpp
TEST(ReferenceCountTest, Referrers) {
    auto mgr = Manager::instance();
    auto taskExp = mgr->task("splitter", "splitter-eng");
    
    ASSERT_TRUE(taskExp.ok());
    auto task = taskExp.value();
    
    auto referrers = task->getReferrers();
    EXPECT_EQ(referrers.size(), task->getReferenceCount());
}
```

---

## 版本历史

- **2.0** (2026-04-05): 优化版本
  - 简化数据结构
  - 去除冗余代码
  - 优化与本地化、UI方案的协同
  - 最小化实现，降低复杂度

- **1.0** (2026-04-05): 初始版本（已废弃）

---

**文档版本**: 2.0
**最后更新**: 2026-04-05
**作者**: Language Manager Team
**状态**: 优化设计阶段