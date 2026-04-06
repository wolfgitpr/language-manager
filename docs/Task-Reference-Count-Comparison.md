# Task 引用计数系统 - 方案对比

## 概述

本文档对比了 Task 引用计数系统的两个设计版本，说明优化方案的改进点和优势。

---

## 版本对比

### 数据结构对比

#### 原方案（v1.0）

```cpp
// 复杂的 ReferenceInfo 结构
struct ReferenceInfo {
    std::string packageId;
    std::string moduleId;
    std::string name;
    std::string category;
    int level;
    
    JsonObject toJson() const;
    bool operator==(const ReferenceInfo &other) const;
};

// ModuleSpec 存储
class ModuleSpec::Impl {
public:
    int refCount = 0;
    std::vector<ReferenceInfo> referrers;
};
```

**问题**：
- ❌ 过度设计，存储了不必要的信息（category, level）
- ❌ 需要实现 toJson() 和比较运算符
- ❌ 数据结构复杂，增加内存开销

#### 优化方案（v2.0）

```cpp
// 简单的 std::pair
class ModuleSpec::Impl {
public:
    int refCount = 0;
    std::vector<std::pair<std::string, std::string>> referrers;  // (packageId, moduleId)
};
```

**优势**：
- ✅ 只存储必要信息（packageId, moduleId）
- ✅ 使用标准库类型，无需额外实现
- ✅ 数据结构简单，内存开销小

---

### 架构对比

#### 原方案（v1.0）

```
PackageManager
    ↓
DependencyGraph (扩展)
    ├── getReferrers()
    ├── calculateRefCount()
    └── updateAllRefCounts()
    ↓
ModuleSpec
    ├── refCount
    ├── referrers (ReferenceInfo[])
    └── getReferenceInfo() (返回复杂 JSON)
    ↓
Task
    ├── getReferenceInfo() (返回复杂 JSON)
    └── getUiSchema() (扩展以包含引用信息)
    ↓
UI Framework
    ├── 解析复杂的 UI Schema
    ├── 显示警告标签
    └── 显示引用者列表
```

**问题**：
- ❌ 扩展了 DependencyGraph，职责不清晰
- ❌ UI Schema 扩展复杂，难以维护
- ❌ 组件之间耦合度高

#### 优化方案（v2.0）

```
PackageManager
    ├── resolveModuleDependencies() (已有)
    └── calculateReferenceCounts() (新增)
    ↓
ModuleSpec
    ├── refCount
    ├── referrers (std::pair[])
    ├── getReferenceCount()
    ├── getReferrers()
    └── isShared()
    ↓
Task
    ├── getReferenceCount() (委托)
    ├── getReferrers() (委托)
    └── isShared() (委托)
    ↓
UI Framework
    ├── 调用 Task API 获取数据
    ├── 基于数据渲染 UI
    └── 显示警告标签
```

**优势**：
- ✅ 职责清晰，每个组件只做一件事
- ✅ 不扩展 DependencyGraph，保持其纯粹性
- ✅ 不扩展 UI Schema，数据驱动渲染
- ✅ 组件之间低耦合

---

### 本地化集成对比

#### 原方案（v1.0）

```json
{
  "reference": {
    "shared": {
      "warning": {
        "_": "This task is used by multiple tasks",
        "zh": "此任务被多个任务使用"
      },
      "details": {
        "0": "English Tagger (eng-official)",
        "1": "Chain English G2p (eng-official)"
      }
    }
  }
}
```

**问题**：
- ❌ 引用者名称硬编码在本地化文件中
- ❌ 无法动态生成引用者列表
- ❌ 维护困难，每次引用者变化都需要更新

#### 优化方案（v2.0）

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

**优势**：
- ✅ 使用占位符，动态替换
- ✅ 引用者列表动态生成
- ✅ 维护简单，只需更新模板

---

### UI Schema 对比

#### 原方案（v1.0）

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
        "message": "reference.shared.warning",
        "details": [
          "reference.shared.details.0",
          "reference.shared.details.1"
        ]
      }
    },
    "sections": [...]
  }
}
```

**问题**：
- ❌ UI Schema 扩展复杂
- ❌ 包含大量冗余信息
- ❌ 需要复杂的解析逻辑
- ❌ 与 UI 模板化系统不兼容

#### 优化方案（v2.0）

```json
{
  "uiSchema": {
    "version": "1.0",
    "sections": [...]
  }
}
```

**优势**：
- ✅ UI Schema 保持简洁
- ✅ 引用信息通过 Task API 获取
- ✅ 数据驱动，无需复杂解析
- ✅ 与 UI 模板化系统兼容

---

### 代码实现对比

#### 原方案（v1.0）

```cpp
// DependencyGraph 扩展
std::vector<ModuleMetadata> DependencyGraph::getReferrers(
    const std::string &packageId,
    const std::string &moduleId) const {
    // 复杂的遍历逻辑...
}

int DependencyGraph::calculateRefCount(
    const std::string &packageId,
    const std::string &moduleId) const {
    // 复杂的遍历逻辑...
}

void DependencyGraph::updateAllRefCounts() {
    // 清空所有引用计数
    for (auto &[key, node] : impl->nodeMap) {
        node->module.refCount = 0;
        node->module.referrers.clear();
    }
    
    // 计算引用关系
    for (auto &[key, node] : impl->nodeMap) {
        for (const auto &dep : node->module.resolvedDependencies) {
            // 复杂的查找和更新逻辑...
        }
    }
}

// ModuleSpec 扩展
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

// Task 扩展
std::string Task::getUiSchema() const {
    // 获取基础 UI Schema
    std::string baseSchema = generateBaseUiSchema();
    
    // 添加引用信息
    auto refInfo = getReferenceInfo();
    if (!refInfo.empty()) {
        auto schemaJson = JsonValue::fromJson(baseSchema);
        auto schemaObj = schemaJson.toObject();
        
        schemaObj["referenceInfo"] = refInfo;
        
        if (isShared()) {
            auto warning = createSharedTaskWarning();
            schemaObj["referenceInfo"]["warning"] = warning;
        }
        
        return schemaObj.toJson();
    }
    
    return baseSchema;
}
```

**问题**：
- ❌ 代码量大，约 200+ 行
- ❌ 逻辑复杂，难以维护
- ❌ 多个组件都需要扩展
- ❌ 性能开销大（多次遍历）

#### 优化方案（v2.0）

```cpp
// PackageManager 扩展
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
        
        for (const auto &dep : moduleInfo.resolvedDependencies) {
            auto depSpec = findModuleSpec(dep.packageId, dep.moduleId);
            if (!depSpec) continue;
            
            depSpec->impl->refCount++;
            depSpec->impl->referrers.emplace_back(
                moduleInfo.packageId,
                moduleInfo.moduleId
            );
        }
    }
}

// ModuleSpec 扩展
int ModuleSpec::getReferenceCount() const {
    return impl->refCount;
}

std::vector<std::pair<std::string, std::string>> ModuleSpec::getReferrers() const {
    return impl->referrers;
}

bool ModuleSpec::isShared() const {
    return impl->refCount > 1;
}

// Task 委托
int Task::getReferenceCount() const {
    if (!impl->spec) return 0;
    return impl->spec->getReferenceCount();
}

std::vector<std::pair<std::string, std::string>> Task::getReferrers() const {
    if (!impl->spec) return {};
    return impl->spec->getReferrers();
}

bool Task::isShared() const {
    if (!impl->spec) return false;
    return impl->spec->isShared();
}
```

**优势**：
- ✅ 代码量少，约 80 行
- ✅ 逻辑简单，易于维护
- ✅ 只扩展必要的组件
- ✅ 性能开销小（单次遍历）

---

### UI 集成对比

#### 原方案（v1.0）

```cpp
void TaskConfigWidget::renderReferenceInfo(const JsonObject &schema) {
    auto refInfo = schema["referenceInfo"].toObject();
    
    // 检查是否被共享
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
}
```

**问题**：
- ❌ 需要解析复杂的 UI Schema
- ❌ 引用者名称硬编码在本地化文件中
- ❌ 无法动态生成引用者列表

#### 优化方案（v2.0）

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

**优势**：
- ✅ 直接调用 Task API，无需解析 Schema
- ✅ 引用者列表动态生成
- ✅ 代码简洁，易于理解

---

## 改进总结

### 主要改进

| 方面 | 改进点 | 优势 |
|------|--------|------|
| **数据结构** | 使用 `std::pair` 替代复杂结构 | 减少内存开销，简化代码 |
| **架构设计** | 不扩展 DependencyGraph | 职责清晰，降低耦合 |
| **UI Schema** | 不扩展 Schema | 保持简洁，易于维护 |
| **本地化** | 使用占位符 | 动态生成，易于维护 |
| **代码量** | 减少 60%+ | 易于维护，降低成本 |
| **性能** | 单次遍历 | 提升性能 |
| **复杂度** | 显著降低 | 易于理解和测试 |

### 去除的冗余

1. ❌ `ReferenceInfo` 结构（5 个字段 → 2 个字段）
2. ❌ `DependencyGraph` 的 3 个扩展方法
3. ❌ 复杂的 UI Schema 扩展
4. ❌ 硬编码的引用者名称
5. ❌ 包卸载时的实时更新
6. ❌ `ReferenceInfo::toJson()` 方法
7. ❌ `ReferenceInfo` 比较运算符
8. ❌ `Task::createSharedTaskWarning()` 方法

### 保留的核心功能

1. ✅ 引用计数计算
2. ✅ 引用者列表
3. ✅ UI 警告显示
4. ✅ 本地化支持
5. ✅ 边界情况处理

---

## 推荐方案

**推荐使用优化方案（v2.0）**，原因：

1. **简洁可靠**：代码量少，逻辑简单
2. **易于维护**：职责清晰，低耦合
3. **性能优异**：单次遍历，内存开销小
4. **兼容性好**：不破坏现有架构
5. **协同性强**：与本地化、UI方案完美集成

---

## 迁移指南

如果已经实现了原方案（v1.0），可以按以下步骤迁移到优化方案（v2.0）：

### 步骤 1：简化数据结构

```cpp
// 删除 ReferenceInfo 结构
// 删除 ReferenceInfo::toJson() 方法
// 删除 ReferenceInfo 比较运算符

// 修改 ModuleSpec::Impl
class ModuleSpec::Impl {
public:
    int refCount = 0;
    std::vector<std::pair<std::string, std::string>> referrers;  // 修改
};
```

### 步骤 2：简化 ModuleSpec

```cpp
// 删除 getReferenceInfo() 方法
// 添加简化方法
int ModuleSpec::getReferenceCount() const;
std::vector<std::pair<std::string, std::string>> ModuleSpec::getReferrers() const;
bool ModuleSpec::isShared() const;
```

### 步骤 3：简化 Task

```cpp
// 修改 Task 方法，使用委托
int Task::getReferenceCount() const;
std::vector<std::pair<std::string, std::string>> Task::getReferrers() const;
bool Task::isShared() const;

// 删除 getReferenceInfo() 方法
// 修改 getUiSchema() 方法，去除引用信息扩展
```

### 步骤 4：移动计算逻辑

```cpp
// 从 DependencyGraph 移除引用计数计算方法
// 在 PackageManager::Impl 中添加 calculateReferenceCounts()
// 在 resolveModuleDependencies() 中调用 calculateReferenceCounts()
```

### 步骤 5：简化本地化

```json
// 删除硬编码的引用者名称
// 使用占位符
{
  "reference": {
    "shared": {
      "warning": "This task is used by {count} tasks",
      "referenced_by": "Referenced by:"
    }
  }
}
```

### 步骤 6：简化 UI

```cpp
// 修改 UI 渲染逻辑
// 直接调用 Task API 获取数据
// 动态生成引用者列表
```

---

## 总结

优化方案（v2.0）在保持核心功能完整的前提下，通过简化数据结构、优化架构设计、减少冗余代码，实现了：

- **代码量减少 60%+**
- **复杂度显著降低**
- **性能提升**
- **维护成本降低**
- **与本地化、UI方案完美集成**

建议采用优化方案（v2.0）进行实现。

---

**文档版本**: 1.0
**最后更新**: 2026-04-05
**作者**: Language Manager Team