# Task 引用计数系统设计文档

## ⚠️ 重要提示

**本文档已废弃**！请使用优化方案 v2.0。

**推荐文档**：
- [Task-Reference-Count-System-Design-Optimized.md](./Task-Reference-Count-System-Design-Optimized.md) - 优化方案 v2.0（推荐）
- [Task-Reference-Count-Comparison.md](./Task-Reference-Count-Comparison.md) - 方案对比（v1.0 vs v2.0）

**废弃原因**：
- 数据结构过于复杂
- 代码量过多（200+ 行）
- 与现有架构耦合度高
- UI Schema 扩展复杂
- 维护成本高

**优化方案改进**：
- 代码量减少 60%+
- 数据结构简化
- 职责更清晰
- 与本地化、UI方案完美集成
- 易于维护

---

## 版本信息

**版本**: 1.0（已废弃）
**创建日期**: 2026-04-05
**作者**: Language Manager Team
**状态**: 已废弃，请使用 v2.0

---

## 文档概述

本文档定义了 Language Manager 的 Task 引用计数系统设计，用于统计和管理 Task 之间的依赖关系，并在 UI 中提供可视化的引用信息。

**注意**：本文档为初始设计方案（v1.0），已被优化方案（v2.0）取代。请参考上述推荐文档。

---

（以下内容仅为存档，请勿使用）

1. [核心概念](#核心概念)
2. [架构设计](#架构设计)
3. [数据结构](#数据结构)
4. [引用计数流程](#引用计数流程)
5. [UI 集成](#ui-集成)
6. [边界情况处理](#边界情况处理)
7. [实现规范](#实现规范)
8. [完整示例](#完整示例)

---

## 核心概念

### 1. 引用关系定义

**引用者（Referrer）**：依赖其他 Task 的 Task
**被引用者（Referent）**：被其他 Task 依赖的 Task

**示例**：
```
Task A (tagger-cmn)
  └─ depends on → Task B (splitter-cmn)

Task C (chain-g2p)
  └─ depends on → Task B (splitter-cmn)

在这种情况下：
- Task A 和 Task C 是引用者
- Task B 是被引用者
- Task B 的引用计数 = 2
```

### 2. 引用计数规则

1. **初始化成功才计数**：只有成功初始化的 Task 才计入引用计数
2. **初始化失败不计数**：初始化失败的 Task 不增加被依赖者的计数
3. **直接依赖才计数**：只统计直接依赖，不递归统计间接依赖
4. **不自动销毁**：引用计数仅用于 UI 显示，不控制 Task 生命周期

### 3. 警告触发条件

**引用计数 > 1**：显示警告标签
**警告内容**：当前 Task 被多个 Task 引用，修改设置可能影响以下 Task：
- 引用者 1 的名称
- 引用者 2 的名称
- ...

---

## 架构设计

### 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    PackageManager                            │
│  - 依赖解析                                                │
│  - 引用关系计算                                            │
│  - 引用计数更新                                            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                 DependencyGraph                              │
│  - 存储所有模块的依赖关系                                  │
│  - 提供反向依赖查询                                        │
│  - 计算引用计数                                            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  ModuleSpec                                  │
│  - refCount: 引用计数                                      │
│  - referrers: 引用者列表                                    │
│  - getReferenceInfo(): 获取引用信息                        │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                      Task                                   │
│  - getUiSchema(): 返回包含引用信息的 UI Schema            │
│  - getReferenceInfo(): 获取引用信息（委托给 ModuleSpec）   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    UI Framework                             │
│  - 解析 UI Schema                                          │
│  - 显示引用计数                                            │
│  - 显示警告标签（如适用）                                  │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件

#### 1. ModuleSpec 扩展

**位置**: `core/lib/Module/Module_p.h`

**新增字段**：
```cpp
class ModuleSpec::Impl {
public:
    // 现有字段...
    int refCount = 0;                           // 引用计数
    std::vector<ReferenceInfo> referrers;        // 引用者列表
};

struct ReferenceInfo {
    std::string packageId;      // 引用者的包 ID
    std::string moduleId;       // 引用者的模块 ID
    std::string name;           // 引用者的名称（DisplayText）
};
```

**新增方法**：
```cpp
// core/include/LangCore/Module/Module.h
class ModuleSpec {
public:
    // 获取引用计数
    int refCount() const;
    
    // 获取引用者列表
    std::vector<ReferenceInfo> referrers() const;
    
    // 检查是否被多个 Task 引用
    bool isShared() const;
    
    // 获取完整的引用信息（用于 UI）
    JsonObject getReferenceInfo() const;
};
```

#### 2. DependencyGraph 扩展

**位置**: `core/include/LangCore/Module/Dependency/DependencyGraph.h`

**新增方法**：
```cpp
class DependencyGraph {
public:
    // 获取指定模块的引用者列表
    std::vector<ModuleMetadata> getReferrers(
        const std::string &packageId,
        const std::string &moduleId
    ) const;
    
    // 计算指定模块的引用计数
    int calculateRefCount(
        const std::string &packageId,
        const std::string &moduleId
    ) const;
    
    // 更新所有模块的引用计数
    void updateAllRefCounts();
};
```

#### 3. Task 扩展

**位置**: `core/include/LangCore/Task/Task.h`

**新增方法**：
```cpp
class Task {
public:
    // 获取引用信息（委托给 ModuleSpec）
    JsonObject getReferenceInfo() const;
    
    // 检查是否被多个 Task 引用
    bool isShared() const;
    
    // 覆盖 getUiSchema() 以包含引用信息
    virtual std::string getUiSchema() const override;
};
```

---

## 数据结构

### 1. ReferenceInfo 结构

```cpp
struct LANGCORE_EXPORT ReferenceInfo {
    std::string packageId;      // 引用者的包 ID
    std::string moduleId;       // 引用者的模块 ID
    std::string name;           // 引用者的名称（DisplayText）
    std::string category;       // 引用者的类别
    int level;                  // 引用者的 API Level
    
    // 转换为 JSON
    JsonObject toJson() const;
    
    // 比较运算符
    bool operator==(const ReferenceInfo &other) const;
};
```

### 2. 引用信息 JSON 结构

```json
{
  "referenceInfo": {
    "refCount": 2,
    "isShared": true,
    "referrers": [
      {
        "packageId": "eng-official",
        "moduleId": "tagger-eng",
        "name": "English Tagger",
        "category": "tagger",
        "level": 1
      },
      {
        "packageId": "chain-official",
        "moduleId": "g2p-chain-eng",
        "name": "Chain English G2p",
        "category": "g2p",
        "level": 1
      }
    ]
  }
}
```

### 3. UI Schema 扩展结构

```json
{
  "uiSchema": {
    "version": "1.0",
    "referenceInfo": {
      "refCount": 2,
      "isShared": true,
      "warning": {
        "type": "shared_task",
        "message": "i18n:reference.shared.warning",
        "details": [
          "i18n:reference.shared.details.0",
          "i18n:reference.shared.details.1"
        ]
      }
    },
    "sections": [...]
  }
}
```

---

## 引用计数流程

### 1. 依赖解析阶段

```
PackageManager::resolveModuleDependencies()
    ↓
DependencyResolver::resolveAllDependencies()
    ↓
// 解析所有模块的依赖关系
for each module in modules:
    resolveDependencies(module)
    ↓
// 依赖解析完成，开始计算引用计数
DependencyGraph::updateAllRefCounts()
```

### 2. 引用计数计算

```cpp
void DependencyGraph::updateAllRefCounts() {
    // 清空所有模块的引用计数
    for (auto &node : nodeMap) {
        node.second->module.refCount = 0;
        node.second->module.referrers.clear();
    }
    
    // 遍历所有模块，计算引用关系
    for (auto &node : nodeMap) {
        const auto &module = node.second->module;
        
        // 遍历该模块的所有依赖
        for (const auto &dep : module.resolvedDependencies) {
            // 找到被依赖的模块
            auto depNode = findNode(dep.packageId, dep.moduleId);
            if (depNode) {
                // 增加引用计数
                depNode->module.refCount++;
                
                // 添加引用者信息
                ReferenceInfo info;
                info.packageId = module.packageId;
                info.moduleId = module.moduleId;
                info.name = module.name;
                info.category = module.type;
                info.level = module.level;
                
                depNode->module.referrers.push_back(info);
            }
        }
    }
}
```

### 3. Task 创建和初始化

```cpp
Expected<NO<Task>> PackageManager::createModuleTask(
    const ModuleMetadata &moduleInfo,
    const Package &pkg) const {
    
    // ... 创建 Task 实例 ...
    
    // 初始化 Task
    auto initResult = task->initialize();
    if (!initResult) {
        // 初始化失败，不增加引用计数
        LOG_ERROR("Failed to initialize task: {}", initResult.error().message());
        return initResult.takeError();
    }
    
    // 初始化成功，注册到 ObjectPool
    auto &ic = *this->category(moduleSpec->category());
    ic.addObject(moduleSpec->id(), task);
    
    // 注意：引用计数已经在依赖解析阶段计算完成
    // 这里不需要更新引用计数
    
    return task;
}
```

### 4. UI Schema 生成

```cpp
std::string Task::getUiSchema() const {
    // 获取基础 UI Schema
    auto baseSchema = generateBaseUiSchema();
    
    // 添加引用信息
    auto refInfo = spec()->getReferenceInfo();
    baseSchema["referenceInfo"] = refInfo;
    
    // 如果被多个 Task 引用，添加警告
    if (spec()->isShared()) {
        auto warning = createSharedTaskWarning();
        baseSchema["referenceInfo"]["warning"] = warning;
    }
    
    return baseSchema.toJson();
}
```

---

## UI 集成

### 1. UI Schema 结构

```json
{
  "uiSchema": {
    "version": "1.0",
    "referenceInfo": {
      "refCount": 2,
      "isShared": true,
      "referrers": [
        {
          "packageId": "eng-official",
          "moduleId": "tagger-eng",
          "name": "English Tagger",
          "category": "tagger",
          "level": 1
        },
        {
          "packageId": "chain-official",
          "moduleId": "g2p-chain-eng",
          "name": "Chain English G2p",
          "category": "g2p",
          "level": 1
        }
      ],
      "warning": {
        "type": "shared_task",
        "message": "i18n:reference.shared.warning",
        "details": [
          "i18n:reference.shared.details.0",
          "i18n:reference.shared.details.1"
        ]
      }
    },
    "sections": [...]
  }
}
```

### 2. 警告标签设计

#### HTML/CSS 实现

```html
<div class="task-warning shared-task-warning">
  <div class="warning-icon">⚠️</div>
  <div class="warning-content">
    <div class="warning-title">
      {{ referenceInfo.warning.message | i18n }}
    </div>
    <div class="warning-details">
      <ul>
        <li v-for="detail in referenceInfo.warning.details">
          {{ detail | i18n }}
        </li>
      </ul>
    </div>
  </div>
</div>
```

#### CSS 样式

```css
.shared-task-warning {
  background-color: #fff3cd;
  border-left: 4px solid #ffc107;
  padding: 12px;
  margin-bottom: 16px;
  border-radius: 4px;
  display: flex;
  align-items: flex-start;
}

.warning-icon {
  font-size: 24px;
  margin-right: 12px;
}

.warning-title {
  font-weight: bold;
  color: #856404;
  margin-bottom: 8px;
}

.warning-details {
  color: #856404;
  font-size: 14px;
}

.warning-details ul {
  margin: 4px 0;
  padding-left: 20px;
}

.warning-details li {
  margin: 4px 0;
}
```

### 3. 本地化文本

**位置**: `plugins/Task/assets/i18n.json`

```json
{
  "$version": "1.0",
  "pluginId": "Task",
  "reference": {
    "shared": {
      "warning": {
        "_": "This task is used by multiple tasks",
        "zh": "此任务被多个任务使用",
        "ja": "このタスクは複数のタスクで使用されています"
      },
      "details": {
        "_": "Modifying settings may affect: {names}",
        "zh": "修改设置可能影响：{names}",
        "ja": "設定の変更は次に影響する可能性があります：{names}"
      }
    }
  }
}
```

### 4. UI 显示逻辑

```cpp
// UI 框架解析引用信息
void TaskConfigWidget::renderTaskReferenceInfo(const JsonObject &schema) {
    auto refInfo = schema["referenceInfo"].toObject();
    
    // 检查是否被多个 Task 引用
    if (refInfo["isShared"].toBool(false)) {
        auto warning = refInfo["warning"].toObject();
        
        // 解析警告消息
        std::string messageKey = warning["message"].toString();
        std::string message = PluginI18nLoader::getPluginText("Task", messageKey);
        
        // 解析详细信息
        auto details = warning["details"].toArray();
        std::vector<std::string> detailMessages;
        
        for (size_t i = 0; i < details.size(); i++) {
            std::string detailKey = details[i].toString();
            std::string detail = PluginI18nLoader::getPluginText("Task", detailKey);
            detailMessages.push_back(detail);
        }
        
        // 显示警告
        ui->showSharedTaskWarning(message, detailMessages);
    }
    
    // 显示引用计数
    int refCount = refInfo["refCount"].toInt(0);
    ui->setReferenceCount(refCount);
    
    // 显示引用者列表
    auto referrers = refInfo["referrers"].toArray();
    for (const auto &referrer : referrers) {
        auto referrerObj = referrer.toObject();
        std::string name = referrerObj["name"].toString();
        std::string category = referrerObj["category"].toString();
        
        ui->addReferrer(name, category);
    }
}
```

---

## 边界情况处理

### 1. 初始化失败

**场景**：Task A 依赖 Task B，但 Task B 初始化失败

**处理方式**：
```cpp
Expected<NO<Task>> PackageManager::createModuleTask(...) {
    // ... 创建 Task 实例 ...
    
    // 初始化 Task
    auto initResult = task->initialize();
    if (!initResult) {
        // 初始化失败
        LOG_ERROR("Failed to initialize task: {}", initResult.error().message());
        
        // 不增加引用计数
        // 不将 Task 注册到 ObjectPool
        // 返回错误
        return initResult.takeError();
    }
    
    // 初始化成功，注册到 ObjectPool
    // 注意：引用计数已经在依赖解析阶段计算完成
    // 此时需要检查被依赖的 Task 是否存在且已初始化
}
```

### 2. 循环依赖

**场景**：Task A 依赖 Task B，Task B 依赖 Task A

**处理方式**：
- 在依赖解析阶段检测循环依赖
- 如果检测到循环依赖，拒绝加载相关包
- 引用计数不会计算循环依赖

```cpp
bool DependencyGraph::hasCycles() const {
    // 使用 Tarjan 算法检测强连通分量
    // 如果存在大小 > 1 的强连通分量，说明有循环依赖
    return ...;
}
```

### 3. 重复依赖

**场景**：Task A 通过不同路径多次依赖 Task B

**处理方式**：
```cpp
// 情况 1：同一个包中多次声明同一个依赖
{
  "dependencies": [
    { "packageId": "eng-official", "moduleId": "splitter-eng" },
    { "packageId": "eng-official", "moduleId": "splitter-eng" }  // 重复
  ]
}
// 处理：去重，只计算一次引用

// 情况 2：通过不同路径间接依赖
// Task A → Task B → Task C
// Task A → Task D → Task C
// 处理：只统计直接依赖，Task C 的引用计数 = 2
```

### 4. 包卸载

**场景**：包含 Task A 的包被卸载

**处理方式**：
- 更新被依赖 Task 的引用计数
- 从被依赖 Task 的引用者列表中移除 Task A
- 如果引用计数降为 0，不自动销毁 Task（仅用于 UI 显示）

```cpp
void PackageManager::unloadPackage(const std::string &packageId) {
    // 获取包中的所有模块
    auto modules = getPackageModules(packageId);
    
    // 更新被依赖模块的引用计数
    for (const auto &module : modules) {
        // 遍历该模块的所有依赖
        for (const auto &dep : module.resolvedDependencies) {
            auto depSpec = findModuleSpec(dep.packageId, dep.moduleId);
            if (depSpec) {
                // 减少引用计数
                depSpec->impl->refCount--;
                
                // 从引用者列表中移除
                removeReferrer(depSpec, module.packageId, module.moduleId);
            }
        }
    }
    
    // 卸载包
    // ...
}
```

### 5. 热重载

**场景**：更新包的依赖关系

**处理方式**：
- 清空所有引用计数
- 重新计算引用关系
- 更新 UI 显示

```cpp
void PackageManager::reloadPackage(const std::string &packageId) {
    // 卸载旧包
    unloadPackage(packageId);
    
    // 加载新包
    loadPackage(packageId);
    
    // 重新计算引用计数
    impl->dependencyGraph->updateAllRefCounts();
    
    // 通知 UI 更新
    notifyUiUpdate();
}
```

---

## 实现规范

### 1. ModuleSpec 实现

```cpp
// core/lib/Module/Module.cpp

int ModuleSpec::refCount() const {
    return impl->refCount;
}

std::vector<ReferenceInfo> ModuleSpec::referrers() const {
    return impl->referrers;
}

bool ModuleSpec::isShared() const {
    return impl->refCount > 1;
}

JsonObject ModuleSpec::getReferenceInfo() const {
    JsonObject info;
    
    info["refCount"] = impl->refCount;
    info["isShared"] = impl->refCount > 1;
    
    JsonArray referrersArray;
    for (const auto &referrer : impl->referrers) {
        referrersArray.push_back(referrer.toJson());
    }
    info["referrers"] = referrersArray;
    
    return info;
}
```

### 2. DependencyGraph 实现

```cpp
// core/lib/Module/Dependency/DependencyGraph.cpp

std::vector<ModuleMetadata> DependencyGraph::getReferrers(
    const std::string &packageId,
    const std::string &moduleId) const {
    
    std::vector<ModuleMetadata> referrers;
    
    // 遍历所有模块
    for (const auto &[key, node] : impl->nodeMap) {
        const auto &module = node->module;
        
        // 检查该模块是否依赖目标模块
        for (const auto &dep : module.resolvedDependencies) {
            if (dep.packageId == packageId && dep.moduleId == moduleId) {
                referrers.push_back(module);
                break;
            }
        }
    }
    
    return referrers;
}

int DependencyGraph::calculateRefCount(
    const std::string &packageId,
    const std::string &moduleId) const {
    
    int count = 0;
    
    // 遍历所有模块
    for (const auto &[key, node] : impl->nodeMap) {
        const auto &module = node->module;
        
        // 检查该模块是否依赖目标模块
        for (const auto &dep : module.resolvedDependencies) {
            if (dep.packageId == packageId && dep.moduleId == moduleId) {
                count++;
                break;
            }
        }
    }
    
    return count;
}

void DependencyGraph::updateAllRefCounts() {
    // 清空所有引用计数
    for (auto &[key, node] : impl->nodeMap) {
        node->module.refCount = 0;
        node->module.referrers.clear();
    }
    
    // 计算引用关系
    for (auto &[key, node] : impl->nodeMap) {
        const auto &module = node->module;
        
        for (const auto &dep : module.resolvedDependencies) {
            auto depNode = findNode(dep.packageId, dep.moduleId);
            if (depNode) {
                // 增加引用计数
                depNode->module.refCount++;
                
                // 添加引用者信息
                ReferenceInfo info;
                info.packageId = module.packageId;
                info.moduleId = module.moduleId;
                info.name = module.name;
                info.category = module.type;
                info.level = module.level;
                
                depNode->module.referrers.push_back(info);
            }
        }
    }
}
```

### 3. Task 实现

```cpp
// core/lib/Task/Task.cpp

JsonObject Task::getReferenceInfo() const {
    if (!impl->spec) {
        return {};
    }
    return impl->spec->getReferenceInfo();
}

bool Task::isShared() const {
    if (!impl->spec) {
        return false;
    }
    return impl->spec->isShared();
}

std::string Task::getUiSchema() const {
    // 获取基础 UI Schema
    std::string baseSchema = generateBaseUiSchema();
    
    // 添加引用信息
    auto refInfo = getReferenceInfo();
    if (!refInfo.empty()) {
        // 解析基础 Schema
        auto schemaJson = JsonValue::fromJson(baseSchema);
        auto schemaObj = schemaJson.toObject();
        
        // 添加引用信息
        schemaObj["referenceInfo"] = refInfo;
        
        // 如果被多个 Task 引用，添加警告
        if (isShared()) {
            auto warning = createSharedTaskWarning();
            schemaObj["referenceInfo"]["warning"] = warning;
        }
        
        return schemaObj.toJson();
    }
    
    return baseSchema;
}

JsonObject Task::createSharedTaskWarning() const {
    JsonObject warning;
    warning["type"] = "shared_task";
    
    // 警告消息
    warning["message"] = "reference.shared.warning";
    
    // 详细信息
    auto refInfo = getReferenceInfo();
    auto referrers = refInfo["referrers"].toArray();
    
    JsonArray details;
    for (size_t i = 0; i < referrers.size(); i++) {
        std::string detailKey = "reference.shared.details." + std::to_string(i);
        details.push_back(detailKey);
    }
    warning["details"] = details;
    
    return warning;
}
```

---

## 完整示例

### 示例场景

假设有以下依赖关系：

```
Package A (eng-official)
  ├─ Module: tagger-eng
  │   └─ depends on: splitter-eng
  │
  └─ Module: g2p-chain-eng
      ├─ depends on: tagger-eng
      └─ depends on: splitter-eng

Package B (cmn-official)
  └─ Module: tagger-cmn
      └─ depends on: splitter-eng
```

### 1. 依赖关系分析

**splitter-eng 的引用者**：
1. tagger-eng (eng-official)
2. g2p-chain-eng (eng-official)
3. tagger-cmn (cmn-official)

**splitter-eng 的引用计数**：3

### 2. UI Schema 生成

```json
{
  "uiSchema": {
    "version": "1.0",
    "referenceInfo": {
      "refCount": 3,
      "isShared": true,
      "referrers": [
        {
          "packageId": "eng-official",
          "moduleId": "tagger-eng",
          "name": "English Tagger",
          "category": "tagger",
          "level": 1
        },
        {
          "packageId": "eng-official",
          "moduleId": "g2p-chain-eng",
          "name": "Chain English G2p",
          "category": "g2p",
          "level": 1
        },
        {
          "packageId": "cmn-official",
          "moduleId": "tagger-cmn",
          "name": "Chinese Tagger",
          "category": "tagger",
          "level": 1
        }
      ],
      "warning": {
        "type": "shared_task",
        "message": "reference.shared.warning",
        "details": [
          "reference.shared.details.0",
          "reference.shared.details.1",
          "reference.shared.details.2"
        ]
      }
    },
    "sections": [
      {
        "id": "basic",
        "title": "i18n:basic.title",
        "fields": [...]
      }
    ]
  }
}
```

### 3. 本地化文件

```json
{
  "$version": "1.0",
  "pluginId": "Splitter",
  "reference": {
    "shared": {
      "warning": {
        "_": "This task is used by multiple tasks",
        "zh": "此任务被多个任务使用"
      },
      "details": {
        "0": {
          "_": "English Tagger (eng-official)",
          "zh": "英语标记器 (eng-official)"
        },
        "1": {
          "_": "Chain English G2p (eng-official)",
          "zh": "责任链英语 G2p (eng-official)"
        },
        "2": {
          "_": "Chinese Tagger (cmn-official)",
          "zh": "中文标记器 (cmn-official)"
        }
      }
    }
  }
}
```

### 4. UI 显示效果

```
┌─────────────────────────────────────────────────────────┐
│ ⚠️  Warning                                             │
│ This task is used by multiple tasks                     │
│ Modifying settings may affect:                         │
│   • English Tagger (eng-official)                       │
│   • Chain English G2p (eng-official)                    │
│   • Chinese Tagger (cmn-official)                       │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ English Text Splitter Configuration                     │
│                                                         │
│ Reference Count: 3                                      │
│                                                         │
│ ┌─ Basic Settings ──────────────────────────────────┐  │
│ │                                                     │  │
│ │ Pattern: ([\p{Han}])                               │  │
│ │                                                     │  │
│ │ [✓] Case Sensitive                                 │  │
│ │                                                     │  │
│ └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 最佳实践

### 1. 性能优化

- **延迟计算**：只在需要时计算引用计数，不在每次依赖解析时都计算
- **缓存结果**：缓存引用计数，只在依赖关系变化时重新计算
- **批量更新**：批量更新多个模块的引用计数，避免重复遍历

### 2. 错误处理

- **日志记录**：记录引用计数计算过程中的错误
- **回退机制**：如果引用计数计算失败，回退到 0
- **用户提示**：如果引用信息不完整，显示警告

### 3. UI 设计

- **简洁明了**：警告信息简洁明了，不占用太多空间
- **可折叠**：引用者列表可折叠，避免界面混乱
- **颜色区分**：使用不同的颜色表示不同的警告级别

### 4. 数据一致性

- **原子更新**：引用计数和引用者列表原子更新
- **事务性**：在热重载时，确保引用计数的一致性
- **验证机制**：定期验证引用计数的正确性

---

## 附录

### A. 引用计数计算算法

```cpp
// O(n²) 算法：遍历所有模块的依赖
void DependencyGraph::updateAllRefCounts_Naive() {
    for (auto &[key, node] : impl->nodeMap) {
        node->module.refCount = 0;
        node->module.referrers.clear();
    }
    
    for (auto &[key, node] : impl->nodeMap) {
        for (const auto &dep : node->module.resolvedDependencies) {
            auto depNode = findNode(dep.packageId, dep.moduleId);
            if (depNode) {
                depNode->module.refCount++;
                // 添加引用者...
            }
        }
    }
}

// O(n) 算法：使用索引加速查找
void DependencyGraph::updateAllRefCounts_Optimized() {
    // 构建反向索引
    std::map<std::string, std::vector<std::string>> reverseIndex;
    
    for (auto &[key, node] : impl->nodeMap) {
        for (const auto &dep : node->module.resolvedDependencies) {
            std::string depKey = dep.packageId + "::" + dep.moduleId;
            reverseIndex[depKey].push_back(key);
        }
    }
    
    // 更新引用计数
    for (auto &[depKey, referrers] : reverseIndex) {
        auto depNode = findNodeByKey(depKey);
        if (depNode) {
            depNode->module.refCount = referrers.size();
            
            // 添加引用者
            for (const auto &referrerKey : referrers) {
                auto referrerNode = findNodeByKey(referrerKey);
                if (referrerNode) {
                    ReferenceInfo info;
                    info.packageId = referrerNode->module.packageId;
                    info.moduleId = referrerNode->module.moduleId;
                    info.name = referrerNode->module.name;
                    // ...
                    
                    depNode->module.referrers.push_back(info);
                }
            }
        }
    }
}
```

### B. 测试用例

```cpp
// 测试引用计数计算
TEST(ReferenceCountTest, CalculateRefCount) {
    // 创建测试模块
    ModuleMetadata moduleA = createModule("A", "pkg-a", "module-a");
    ModuleMetadata moduleB = createModule("B", "pkg-b", "module-b");
    ModuleMetadata moduleC = createModule("C", "pkg-c", "module-c");
    
    // 设置依赖关系
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    moduleC.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // 构建依赖图
    DependencyGraph graph;
    graph.addNode(moduleA);
    graph.addNode(moduleB);
    graph.addNode(moduleC);
    
    // 计算引用计数
    graph.updateAllRefCounts();
    
    // 验证
    EXPECT_EQ(moduleB.refCount, 2);
    EXPECT_EQ(moduleB.referrers.size(), 2);
    EXPECT_EQ(moduleB.referrers[0].moduleId, "module-a");
    EXPECT_EQ(moduleB.referrers[1].moduleId, "module-c");
}

// 测试初始化失败不影响引用计数
TEST(ReferenceCountTest, InitFailure) {
    // ... 测试代码 ...
}

// 测试循环依赖检测
TEST(ReferenceCountTest, CircularDependency) {
    // ... 测试代码 ...
}
```

---

## 版本历史

- **1.0** (2026-04-05): 初始版本
  - 定义引用计数系统
  - 设计数据结构和算法
  - 实现 UI 集成方案
  - 提供完整示例和最佳实践

---

**文档版本**: 1.0
**最后更新**: 2026-04-05
**作者**: Language Manager Team
**状态**: 设计阶段