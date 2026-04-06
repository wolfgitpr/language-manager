# Task 引用计数系统快速开始指南

本指南帮助开发者快速上手 Task 引用计数系统（优化方案 v2.0）。

## 基础概念

### 什么是引用计数？

引用计数表示一个 Task 被其他 Task 引用的次数。

**示例**：
```
Task A (tagger-eng)
  └─ depends on → Task B (splitter-eng)

Task C (chain-g2p)
  └─ depends on → Task B (splitter-eng)

Task B 的引用计数 = 2（被 A 和 C 引用）
```

### 为什么需要引用计数？

- **UI 警告**：当 Task 被多个 Task 引用时，显示警告标签
- **影响提示**：提示用户修改设置可能影响多个 Task
- **依赖分析**：帮助理解 Task 之间的依赖关系

## 快速开始

### 1. 检查 Task 是否被共享

```cpp
#include <LangCore/Core/Manager.h>

// 获取 Manager 实例
auto mgr = LangCore::Manager::instance();

// 获取 Task
auto taskExp = mgr->task("splitter", "splitter-eng");
if (!taskExp) {
    // 处理错误
    return;
}

auto task = taskExp.value();

// 检查是否被多个 Task 引用
if (task->isShared()) {
    int refCount = task->getReferenceCount();
    std::cout << "⚠️  This task is shared by " << refCount << " tasks" << std::endl;
    
    // 获取引用者列表
    auto referrers = task->getReferrers();
    for (const auto &referrer : referrers) {
        std::cout << "  • " << referrer.first << "::" << referrer.second << std::endl;
    }
}
```

### 2. 获取引用信息

```cpp
// 获取引用计数
int refCount = task->getReferenceCount();

// 获取引用者列表
auto referrers = task->getReferrers();
for (const auto &referrer : referrers) {
    std::string packageId = referrer.first;
    std::string moduleId = referrer.second;
    
    std::cout << packageId << "::" << moduleId << std::endl;
}
```

### 3. 在 UI 中显示警告

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

## UI 集成

### 显示警告标签

当 Task 被多个 Task 引用时，在 UI 中显示警告标签：

```cpp
void TaskConfigWidget::renderReferenceWarning(Task *task)
{
    // 检查是否被共享
    if (!task->isShared()) {
        return;
    }
    
    // 显示警告
    ui->showWarning("This task is used by multiple tasks");
    
    // 显示引用者列表
    auto referrers = task->getReferrers();
    for (const auto &referrer : referrers) {
        auto name = getTaskName(referrer.first, referrer.second);
        ui->addReferrer(name);
    }
}
```

### 本地化文本

**位置**: `plugins/Task/assets/i18n.json`

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

### 占位符替换

```cpp
// 简单的占位符替换实现
std::string replacePlaceholder(
    const std::string &text,
    const std::string &placeholder,
    const std::string &value)
{
    std::string result = text;
    size_t pos = result.find(placeholder);
    
    while (pos != std::string::npos) {
        result.replace(pos, placeholder.length(), value);
        pos = result.find(placeholder, pos + value.length());
    }
    
    return result;
}
```

## 常见场景

### 场景 1：检查所有共享的 Task

```cpp
void showAllSharedTasks()
{
    auto mgr = LangCore::Manager::instance();
    
    // 获取所有类别
    auto categories = mgr->categories();
    
    for (const auto &category : categories) {
        auto specs = category->specs();
        
        for (const auto &spec : specs) {
            if (spec->isShared()) {
                std::cout << "⚠️  " << spec->id() 
                         << " (refCount: " << spec->getReferenceCount() << ")" 
                         << std::endl;
            }
        }
    }
}
```

### 场景 2：在修改配置前检查影响

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
        std::cout << "⚠️  Warning: This task is shared by " 
                  << task->getReferenceCount() << " tasks" << std::endl;
        
        // 显示引用者
        auto referrers = task->getReferrers();
        for (const auto &referrer : referrers) {
            auto name = getTaskName(referrer.first, referrer.second);
            std::cout << "  • " << name << std::endl;
        }
        
        // 确认修改
        std::cout << "Continue? (y/n): ";
        char choice;
        std::cin >> choice;
        
        if (choice != 'y' && choice != 'Y') {
            return {};
        }
    }
    
    // 修改配置
    return task->setConfig(newConfig);
}
```

### 场景 3：分析依赖关系

```cpp
void analyzeDependencies(const std::string &category, const std::string &id)
{
    auto mgr = LangCore::Manager::instance();
    auto taskExp = mgr->task(category, id);
    
    if (!taskExp) {
        return;
    }
    
    auto task = taskExp.value();
    
    // 显示引用计数
    int refCount = task->getReferenceCount();
    std::cout << "Task: " << task->spec()->id() << std::endl;
    std::cout << "Reference Count: " << refCount << std::endl;
    
    // 显示引用者
    auto referrers = task->getReferrers();
    if (!referrers.empty()) {
        std::cout << "Referenced by:" << std::endl;
        for (const auto &referrer : referrers) {
            auto name = getTaskName(referrer.first, referrer.second);
            std::cout << "  • " << name << std::endl;
        }
    }
}
```

## 边界情况

### 1. 初始化失败

初始化失败的 Task 不会增加被依赖者的引用计数。

```cpp
// Task A 依赖 Task B
// 如果 Task A 初始化失败，Task B 的引用计数不会增加
```

### 2. 循环依赖

循环依赖会被检测并拒绝，不会计算引用计数。

```cpp
// A → B → C → A
// 这会形成循环依赖，系统会拒绝加载这些模块
```

### 3. 重复依赖

重复的依赖会被去重，只计算一次。

```json
{
  "dependencies": [
    { "packageId": "pkg-b", "moduleId": "module-b" },
    { "packageId": "pkg-b", "moduleId": "module-b" }  // 重复
  ]
}
// 引用计数 = 1
```

### 4. 间接依赖

只统计直接依赖，不统计间接依赖。

```
A → B → C
// C 的引用计数 = 1（被 B 依赖）
// C 的引用计数 ≠ 2（不被 A 间接依赖）
```

## 最佳实践

### 1. 在 UI 中显示警告

```cpp
// 如果 Task 被共享，始终显示警告
if (task->isShared()) {
    ui->showSharedTaskWarning(task->getReferenceCount());
}
```

### 2. 在修改配置前确认

```cpp
// 修改配置前检查引用计数
if (task->isShared() && refCount > 1) {
    if (!confirm("This task is shared. Continue?")) {
        return;
    }
}
```

### 3. 记录引用信息

```cpp
// 在日志中记录引用信息
if (task->isShared()) {
    LOG_WARNING("Task {} is shared by {} tasks", 
               task->spec()->id(), 
               task->getReferenceCount());
    
    auto referrers = task->getReferrers();
    for (const auto &referrer : referrers) {
        LOG_INFO("  Referenced by: {}::{}", referrer.first, referrer.second);
    }
}
```

### 4. 提供撤销选项

```cpp
// 如果 Task 被共享，提供撤销选项
if (task->isShared()) {
    ui->enableUndoButton();
}
```

## 完整示例

### Task 配置界面

```cpp
class TaskConfigWidget : public QWidget
{
public:
    TaskConfigWidget(const std::string &category, const std::string &id)
    {
        auto mgr = LangCore::Manager::instance();
        auto taskExp = mgr->task(category, id);
        
        if (!taskExp) {
            showError("Failed to load task");
            return;
        }
        
        m_task = taskExp.value();
        
        // 加载 UI Schema
        loadUiSchema();
        
        // 显示引用信息
        showReferenceInfo();
        
        // 渲染配置表单
        renderConfigForm();
    }
    
private:
    void showReferenceInfo()
    {
        if (!m_task->isShared()) {
            return;
        }
        
        int refCount = m_task->getReferenceCount();
        
        // 创建警告标签
        auto *warningLabel = new QLabel(this);
        warningLabel->setText(
            QString("⚠️  This task is used by %1 tasks").arg(refCount)
        );
        warningLabel->setStyleSheet(
            "background-color: #fff3cd;"
            "border-left: 4px solid #ffc107;"
            "padding: 8px;"
            "border-radius: 4px;"
        );
        
        // 创建引用者列表
        auto *referrersList = new QListWidget(this);
        auto referrers = m_task->getReferrers();
        for (const auto &referrer : referrers) {
            auto name = getTaskName(referrer.first, referrer.second);
            auto packageId = QString::fromStdString(referrer.first);
            auto moduleId = QString::fromStdString(referrer.second);
            
            auto itemText = QString("%1 (%2::%3)")
                .arg(name)
                .arg(packageId)
                .arg(moduleId);
            
            referrersList->addItem(itemText);
        }
        
        // 添加到布局
        layout()->addWidget(warningLabel);
        layout()->addWidget(referrersList);
    }
    
    std::string getTaskName(const std::string &packageId, const std::string &moduleId)
    {
        auto mgr = LangCore::Manager::instance();
        auto spec = mgr->findModuleSpec(packageId, moduleId);
        
        if (!spec) {
            return packageId + "::" + moduleId;
        }
        
        // 返回本地化后的名称
        LangCore::DisplayText name(spec->name());
        return name.text();
    }
    
    void loadUiSchema()
    {
        std::string uiSchema = m_task->getUiSchema();
        // 解析并渲染 UI Schema...
    }
    
    void renderConfigForm()
    {
        // 渲染配置表单...
    }
    
    LangCore::NO<LangCore::Task> m_task;
};
```

## 相关文档

- [Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md) - 完整设计文档（优化方案 v2.0）
- [Task-Reference-Count-Comparison.md](../Task-Reference-Count-Comparison.md) - 方案对比（v1.0 vs v2.0）
- [task-reference-count-examples.cpp](./task-reference-count-examples.cpp) - 代码示例
- [task-reference-count-tests.cpp](./task-reference-count-tests.cpp) - 测试用例

## 常见问题

### Q1: 引用计数会影响 Task 的生命周期吗？

**A**: 不会。引用计数仅用于 UI 显示，不影响 Task 的自动销毁。

### Q2: 如何重置引用计数？

**A**: 引用计数由系统自动计算。如果需要重新计算，可以重新解析依赖：
```cpp
mgr->resolveModuleDependencies();
```

### Q3: 引用计数为零意味着什么？

**A**: 意味着该 Task 不被任何其他 Task 依赖。这通常是正常的。

### Q4: 如何处理循环依赖？

**A**: 循环依赖会在依赖解析阶段被检测并拒绝。需要修改依赖配置以消除循环。

### Q5: 优化方案（v2.0）与原方案（v1.0）有什么区别？

**A**: 优化方案（v2.0）在保持核心功能完整的前提下，通过简化数据结构、优化架构设计、减少冗余代码，实现了代码量减少 60%+、复杂度显著降低、性能提升。详见[方案对比文档](../Task-Reference-Count-Comparison.md)。

---

**版本**: 2.0
**最后更新**: 2026-04-05