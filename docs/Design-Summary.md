# Language Manager 设计优化总结

## 概述

本文档总结了 Language Manager 项目近期完成的设计工作，包括本地化系统、UI 模板化系统和 Task 引用计数系统的优化过程。

---

## 设计文档列表

### 1. 本地化系统

**文档**: [Localization-Design.md](./Localization-Design.md)

**核心功能**：
- 插件自带本地化（i18n.json）
- 包级别本地化（package.json）
- ChainG2p 特殊本地化
- 松耦合架构

**特点**：
- 职责分离：插件开发者 vs 包创建者
- 配置简便：最小化用户配置负担
- 回退策略：多级回退机制

### 2. UI 模板化系统

**文档**: [UI-Template-System-Design.md](./UI-Template-System-Design.md)

**核心功能**：
- 宏定义声明 UI 结构
- 自动生成 JSON
- 自动本地化键名生成
- ChainG2p 特殊支持

**特点**：
- 声明式编程
- 类型安全
- 架构统一
- 与本地化系统无缝集成

### 3. 集成化系统

**文档**: [Integrated-Localization-UI-System-Design.md](./Integrated-Localization-UI-System-Design.md)

**核心内容**：
- 两个系统的协同工作方式
- 统一键名规范
- 完整的实现流程
- 一般插件和 ChainG2p 的支持

**特点**：
- 完全集成
- 自动化处理
- 数据驱动
- 简化开发

### 4. Task 引用计数系统

**文档**: 
- [Task-Reference-Count-System-Design-Optimized.md](./Task-Reference-Count-System-Design-Optimized.md) - 优化方案 v2.0（推荐）
- [Task-Reference-Count-Comparison.md](./Task-Reference-Count-Comparison.md) - 方案对比

**核心功能**：
- 引用计数计算
- 引用者列表
- UI 警告显示
- 边界情况处理

**优化改进**：
- 代码量减少 60%+
- 数据结构简化
- 职责清晰
- 与本地化、UI方案完美集成

---

## 设计优化过程

### 初始方案（v1.0）

**特点**：
- 功能完整
- 代码复杂
- 数据结构复杂
- UI Schema 扩展复杂

**问题**：
- ❌ 代码量过多（200+ 行）
- ❌ 数据结构过度设计
- ❌ 与现有架构耦合度高
- ❌ 维护成本高

### 优化方案（v2.0）

**改进点**：
- ✅ 简化数据结构
- ✅ 减少代码量
- ✅ 优化架构设计
- ✅ 提升性能
- ✅ 增强协同性

**具体改进**：
1. **数据结构简化**：`ReferenceInfo` → `std::pair`
2. **架构优化**：不扩展 DependencyGraph
3. **UI Schema 简化**：数据驱动，不扩展 Schema
4. **本地化优化**：使用占位符，动态生成
5. **代码量减少**：200+ 行 → 80 行

---

## 系统协同性

### 本地化 ↔ UI 模板化

**协同方式**：
- UI 宏自动生成本地化键名
- 生成的 JSON 自动包含 `i18n:` 前缀
- 无缝集成，无需手动配置

**示例**：
```cpp
// 宏声明
LANGCORE_UI_FIELD_SLIDER(threshold, 0.0, 1.0, 0.01, 0.5)

// 自动生成键名
// - threshold.label
// - threshold.description

// 自动生成 JSON
{
  "label": "i18n:threshold.label",
  "description": "i18n:threshold.description"
}
```

### 本地化/UI ↔ Task 引用计数

**协同方式**：
- 引用信息通过 Task API 获取
- UI 基于数据渲染，不依赖复杂 Schema
- 本地化使用占位符，动态生成

**示例**：
```cpp
// 获取引用信息
int refCount = task->getReferenceCount();
auto referrers = task->getReferrers();

// 本地化
std::string warning = replacePlaceholder(
    "This task is used by {count} tasks",
    "{count}",
    std::to_string(refCount)
);

// UI 显示
ui->showWarning(warning);
```

---

## 设计原则

### 1. 简洁可靠

- 最小化设计
- 避免过度工程
- 简化实现

### 2. 职责单一

- 每个组件只做一件事
- 明确的责任边界
- 低耦合高内聚

### 3. 非侵入性

- 不破坏现有架构
- 向后兼容
- 平滑升级

### 4. 数据驱动

- 基于数据渲染 UI
- 不依赖复杂逻辑
- 易于维护

### 5. 延迟计算

- 只在需要时计算
- 缓存结果
- 提升性能

---

## 最佳实践

### 1. 命名规范

```cpp
// 字段级键名
{fieldName}.label
{fieldName}.description

// 选项级键名
{optionValue}.label
{optionValue}.description

// Section 级键名
{sectionId}.title
{sectionId}.description
```

### 2. 占位符使用

```json
{
  "reference": {
    "shared": {
      "warning": "This task is used by {count} tasks"
    }
  }
}
```

### 3. 数据驱动 UI

```cpp
// 获取数据
int refCount = task->getReferenceCount();
auto referrers = task->getReferrers();

// 基于数据渲染
ui->showWarning(getWarningText(refCount));
ui->showReferrerList(getReferrerNames(referrers));
```

### 4. 延迟计算

```cpp
// 只在依赖解析完成时计算一次
void PackageManager::Impl::calculateReferenceCounts() {
    // 计算逻辑...
}
```

---

## 文档索引

### 核心设计文档

1. [Integrated-Localization-UI-System-Design.md](./Integrated-Localization-UI-System-Design.md) - **推荐首先阅读**
2. [Localization-Design.md](./Localization-Design.md) - 本地化系统
3. [UI-Template-System-Design.md](./UI-Template-System-Design.md) - UI 模板化系统
4. [Task-Reference-Count-System-Design-Optimized.md](./Task-Reference-Count-System-Design-Optimized.md) - Task 引用计数（v2.0）
5. [Task-Reference-Count-Comparison.md](./Task-Reference-Count-Comparison.md) - 方案对比

### 废弃文档

- [Task-Reference-Count-System-Design.md](./Task-Reference-Count-System-Design.md) - v1.0（已废弃）

### 示例文件

位于 `docs/examples/` 目录：
- `i18n-example.json` - 本地化示例
- `package-example.json` - 包配置示例
- `ui-macro-example.cpp` - UI 宏使用示例
- `task-reference-count-examples.cpp` - 引用计数示例
- `task-reference-count-tests.cpp` - 引用计数测试
- `Task-Reference-Count-Quick-Start.md` - 快速开始指南

### 配置规范

- [Plugin-UI-Configuration-Schema.md](./Plugin-UI-Configuration-Schema.md) - UI 配置模式规范

---

## 开发建议

### 对于插件开发者

1. 阅读 [Integrated-Localization-UI-System-Design.md](./Integrated-Localization-UI-System-Design.md)
2. 使用 UI 宏定义声明配置界面
3. 创建 `assets/i18n.json` 提供本地化
4. 参考 `ui-macro-example.cpp`

### 对于包创建者

1. 阅读 [Localization-Design.md](./Localization-Design.md)
2. 在 `package.json` 中提供包级别本地化
3. 如果包含 ChainG2p，添加 `stepDescriptions` 字段
4. 参考 `package-example.json`

### 对于 UI 开发者

1. 阅读 [Integrated-Localization-UI-System-Design.md](./Integrated-Localization-UI-System-Design.md)
2. 使用数据驱动方式渲染 UI
3. 解析 `i18n:` 前缀的本地化键名
4. 显示引用警告（如适用）

---

## 版本历史

- **2026-04-05**: 完成本地化系统、UI 模板化系统、Task 引用计数系统的设计和优化

---

**文档版本**: 1.0
**最后更新**: 2026-04-05
**作者**: Language Manager Team