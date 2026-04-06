# Examples

本目录包含 Language Manager 的示例文件，帮助开发者理解和使用各种系统功能。

## 文件说明

### 本地化示例

#### i18n-example.json

插件的本地化文件示例，展示了：

- UI Schema 字段的本地化（label、description、placeholder）
- 选项的本地化（options）
- Section 的本地化（title、description）
- 单位的本地化（unit）

**适用场景**：
- 插件开发者创建自己的 i18n.json
- UI 开发者理解本地化键名结构

**位置**：`plugins/{PluginName}/assets/i18n.json`

#### package-example.json

包的本地化文件示例，展示了：

- 包级别的本地化（vendor、copyright、description）
- 模块级别的本地化（name、description）
- ChainG2p 步骤描述的本地化（stepDescriptions）

**适用场景**：
- 包创建者创建 package.json
- 理解 ChainG2p 的特殊本地化处理

**位置**：`res/G2pPackages/{PackageName}/package.json`

### Task 引用计数示例

#### task-reference-count-examples.cpp

Task 引用计数系统使用示例（基于优化方案 v2.0），展示了：

- 检查 Task 是否被多个 Task 引用
- 获取引用信息并显示警告
- 在 Task 配置界面中显示引用信息
- 处理包卸载时的引用计数更新
- 遍历所有 Task 并显示引用统计
- 处理热重载时的引用计数重新计算
- 自定义 Task 实现中的引用信息处理
- 验证引用计数计算的正确性
- 显示引用关系图

**适用场景**：
- 理解引用计数系统的使用方式
- 学习如何处理各种边界情况
- 参考实现自定义的引用信息处理

**相关文档**：[Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md)（推荐）

**注意**：原方案（v1.0）已废弃，建议使用优化方案（v2.0）。详见[方案对比文档](../Task-Reference-Count-Comparison.md)。

#### task-reference-count-tests.cpp

Task 引用计数系统测试用例（基于优化方案 v2.0），包含了简化的测试用例，覆盖了：

- 基本的引用计数计算
- 检查 Task 是否被共享
- 获取引用者列表
- 初始化失败不影响引用计数
- 循环依赖检测
- 重复依赖只计算一次
- 间接依赖不计入引用计数
- UI 显示测试

**适用场景**：
- 验证引用计数系统的正确性
- 理解各种边界情况的处理方式
- 作为编写新测试的参考

**相关文档**：[Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md)（推荐）

**注意**：原方案（v1.0）已废弃，建议使用优化方案（v2.0）。详见[方案对比文档](../Task-Reference-Count-Comparison.md)。

### UI 模板化示例

#### ui-macro-example.cpp

UI 模板化宏的使用示例，展示了：

- 基础控件宏的使用（slider、text、checkbox、radio、dropdown、spinbox、label）
- Section 的组织方式
- ChainG2p 特殊 Group 宏的使用
- 复杂 UI 的声明方式
- 可选参数的使用

**包含示例**：
1. MandarinG2p UI 声明
2. ChainG2p UI 声明
3. RegexSplitter UI 声明
4. TemplateTagger UI 声明
5. 复杂的 Section 组织
6. 使用标签和描述性字段

**适用场景**：
- 插件开发者学习如何使用宏定义
- 理解不同控件的声明方式
- 学习 ChainG2p 的特殊 UI 处理

**相关文档**：[UI-Template-System-Design.md](../UI-Template-System-Design.md)

### 头文件示例

#### UiMacros.h.example

展示所有 UI 宏的定义：

- `LANGCORE_UI_SCHEMA`
- `LANGCORE_UI_SECTION`
- `LANGCORE_UI_OPTION`
- `LANGCORE_UI_FIELD_SLIDER`
- `LANGCORE_UI_FIELD_TEXT`
- `LANGCORE_UI_FIELD_CHECKBOX`
- `LANGCORE_UI_FIELD_RADIO`
- `LANGCORE_UI_FIELD_DROPDOWN`
- `LANGCORE_UI_FIELD_SPINBOX`
- `LANGCORE_UI_FIELD_LABEL`

#### ChainG2pMacros.h.example

展示 ChainG2p 专用宏：

- `LANGCORE_CHAIN_G2P_SCHEMA`
- `LANGCORE_UI_FIELD_GROUP`
- `LANGCORE_CHAIN_G2P_SECTION_STEPS`
- 简化步骤宏：`LANGCORE_CHAIN_G2P_STEP_*`

#### UiSchema.h.example

展示数据结构定义：

- `UiFieldType` 枚举
- `UiOption` 结构
- `UiField` 结构
- `UiSection` 结构
- `UiSchema` 结构

#### UiBuilder.h.example

展示构建器接口：

- `build()` - 构建 Schema
- `toJson()` - 生成 JSON
- `validate()` - 验证 Schema
- `fromJson()` - 解析 JSON

### 快速开始指南

#### Task-Reference-Count-Quick-Start.md

Task 引用计数系统快速开始指南（基于优化方案 v2.0），包含：

- 基础概念介绍
- 快速开始示例
- UI 集成指南
- 常见场景示例
- 边界情况处理
- 最佳实践
- 完整示例代码
- 常见问题解答

**适用场景**：
- 快速上手引用计数系统
- 学习基本用法
- 解决常见问题

**相关文档**：
- [Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md) - 完整设计文档
- [Task-Reference-Count-Comparison.md](../Task-Reference-Count-Comparison.md) - 方案对比

## 更多信息

详细的设计文档请参阅：
- [Design-Summary.md](../Design-Summary.md) - **设计优化总结（推荐首先阅读）**
- [Integrated-Localization-UI-System-Design.md](../Integrated-Localization-UI-System-Design.md) - **集成化本地化与 UI 模板化系统设计**
- [Localization-Design.md](../Localization-Design.md) - 本地化系统设计
- [UI-Template-System-Design.md](../UI-Template-System-Design.md) - UI 模板化系统设计
- [Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md) - **Task 引用计数系统设计（优化方案 v2.0，推荐）**
- [Task-Reference-Count-Comparison.md](../Task-Reference-Count-Comparison.md) - Task 引用计数系统方案对比（v1.0 vs v2.0）
- [Plugin-UI-Configuration-Schema.md](../Plugin-UI-Configuration-Schema.md) - UI 配置模式规范

**阅读建议**：
1. 先阅读 [Design-Summary.md](../Design-Summary.md) 了解整体设计
2. 再阅读 [Integrated-Localization-UI-System-Design.md](../Integrated-Localization-UI-System-Design.md) 了解协同工作方式
3. 根据需要阅读具体系统的详细文档

**注意**：
- 建议先阅读集成化系统设计文档，了解本地化系统和 UI 模板化系统如何协同工作
- Task 引用计数系统（v2.0）与本地化、UI方案完美集成，采用数据驱动设计
- 原方案（v1.0）已废弃，详见方案对比文档

## 快速开始

### 本地化快速开始

1. 在插件目录下创建 `assets/i18n.json`
2. 参考 `i18n-example.json` 编写本地化内容
3. 在插件初始化时加载本地化文件

### UI 模板化快速开始

1. 包含 `LangCore/Support/UiMacros.h`
2. 使用宏定义声明 UI Schema
3. 调用 `UiBuilder::toJson()` 生成 JSON

### Task 引用计数快速开始

1. 获取 Task 实例
2. 检查 `task->isShared()` 是否为 true
3. 获取引用计数和引用者列表

**详细指南**：
- [Task-Reference-Count-Quick-Start.md](./Task-Reference-Count-Quick-Start.md) - Task 引用计数快速开始

## 贡献

如果您有改进建议或发现问题，请提交 Issue 或 Pull Request。

---

**最后更新**: 2026-04-05
**版本**: 1.0

#### UiMacros.h.example

UI 模板化宏的定义示例，展示了：

- UI Schema 宏定义
- Section 宏定义
- Option 宏定义
- 各种 Field 宏定义（slider、text、checkbox、radio、dropdown、spinbox、label）

**适用场景**：
- 理解宏的内部实现
- 学习如何扩展宏定义
- 参考实现自定义宏

#### ChainG2pMacros.h.example

ChainG2p 专用宏的定义示例，展示了：

- ChainG2p Schema 宏定义
- Group 宏定义（用于包装每个 step）
- ChainG2p 特殊 Section 宏
- 常用步骤的简化宏（clean、tag_validate、dict、model、fallback）

**适用场景**：
- 理解 ChainG2p 的特殊处理
- 学习如何声明 ChainG2p 步骤
- 参考实现自定义步骤宏

#### UiSchema.h.example

UI Schema 数据结构定义示例，展示了：

- UiFieldType 枚举（字段类型）
- UiOption 结构（选项）
- UiField 结构（字段）
- UiSection 结构（Section）
- UiSchema 结构（Schema）

**适用场景**：
- 理解 UI Schema 的数据结构
- 学习如何使用数据结构
- 参考实现自定义数据结构

#### UiBuilder.h.example

UI 构建器的定义示例，展示了：

- UiBuilder 类接口
- build 方法（构建 Schema）
- toJson 方法（生成 JSON）
- validate 方法（验证 Schema）
- fromJson 方法（解析 JSON）

**适用场景**：
- 理解 UI 构建器的实现
- 学习如何生成 JSON
- 参考实现自定义构建器

## 快速开始

### 1. 使用 UI 模板化宏

```cpp
#include <LangCore/Support/UiMacros.h>

Expected<std::string> MyTask::getUiSchema() const {
    auto schema = LANGCORE_UI_SCHEMA(
        "MyPlugin",
        LANGCORE_UI_SECTION(
            basic,
            LANGCORE_UI_FIELD_SLIDER(threshold, 0.0, 1.0, 0.01, 0.5),
            LANGCORE_UI_FIELD_CHECKBOX(debugMode, false)
        )
    );

    return UiBuilder::toJson(schema);
}
```

### 2. 创建本地化文件

参考 `i18n-example.json` 创建 `assets/i18n.json`：

```json
{
  "$version": "1.0",
  "pluginId": "MyPlugin",
  "uiSchema": {
    "fields": {
      "threshold": {
        "label": {
          "_": "Confidence Threshold",
          "zh": "置信度阈值"
        }
      }
    },
    "sections": {
      "basic": {
        "title": {
          "_": "Basic Settings",
          "zh": "基本设置"
        }
      }
    }
  }
}
```

### 3. 创建包本地化

参考 `package-example.json` 更新 `package.json`：

```json
{
  "packageId": "my-package",
  "vendor": {
    "_": "My Name",
    "zh": "我的名字"
  },
  "stepDescriptions": {
    "clean": {
      "_": "Preprocess text",
      "zh": "预处理文本"
    }
  }
}
```

## 更多信息

详细的设计文档请参阅：
- [Integrated-Localization-UI-System-Design.md](../Integrated-Localization-UI-System-Design.md) - **集成化本地化与 UI 模板化系统设计（推荐先阅读）**
- [Localization-Design.md](../Localization-Design.md) - 本地化系统设计
- [UI-Template-System-Design.md](../UI-Template-System-Design.md) - UI 模板化系统设计
- [Task-Reference-Count-System-Design-Optimized.md](../Task-Reference-Count-System-Design-Optimized.md) - **Task 引用计数系统设计（优化方案 v2.0，推荐）**
- [Task-Reference-Count-Comparison.md](../Task-Reference-Count-Comparison.md) - Task 引用计数系统方案对比（v1.0 vs v2.0）
- [Plugin-UI-Configuration-Schema.md](../Plugin-UI-Configuration-Schema.md) - UI 配置模式规范

**注意**：
- 建议先阅读集成化系统设计文档，了解本地化系统和 UI 模板化系统如何协同工作
- Task 引用计数系统（v2.0）与本地化、UI方案完美集成，采用数据驱动设计
- 原方案（v1.0）已废弃，详见方案对比文档

## 快速开始

### 本地化快速开始

1. 在插件目录下创建 `assets/i18n.json`
2. 参考 `i18n-example.json` 编写本地化内容
3. 在插件初始化时加载本地化文件

### UI 模板化快速开始

1. 包含 `LangCore/Support/UiMacros.h`
2. 使用宏定义声明 UI Schema
3. 调用 `UiBuilder::toJson()` 生成 JSON

### Task 引用计数快速开始

1. 获取 Task 实例
2. 检查 `task->isShared()` 是否为 true
3. 获取引用计数和引用者列表

**详细指南**：
- [Task-Reference-Count-Quick-Start.md](./Task-Reference-Count-Quick-Start.md) - Task 引用计数快速开始

## 贡献

如果您有改进建议或发现问题，请提交 Issue 或 Pull Request。

---

**最后更新**: 2026-04-05
**版本**: 1.0

## 贡献

如果您有改进建议或发现问题，请提交 Issue 或 Pull Request。

---

**最后更新**: 2026-04-05
**版本**: 1.0