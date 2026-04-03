# Plugin UI Configuration Schema

## 概述

本文档定义了插件配置界面的 JSON 声明规范，用于生成从上到下的列表式配置界面。

**设计原则**：
- 简洁：只声明类型和值，不考虑 layout 和样式
- 实用：聚焦实际使用场景
- 类型安全：每个控件都有明确的类型定义

---

## Schema 结构

### 根对象

```json
{
  "uiSchema": {
    "version": "1.0",
    "sections": [...]
  }
}
```

**字段说明**：
- `version`: Schema 版本号
- `sections`: 配置分段列表

---

## 支持的控件类型

### 1. 文本框 (text)

单行文本输入控件。

**字段定义**：
```json
{
  "id": "pattern",
  "type": "text",
  "label": "Pattern",
  "description": "Enter the regex pattern for splitting",
  "value": "([\\p{Han}])",
  "placeholder": "Enter pattern...",
  "required": true,
  "validation": {
    "minLength": 1,
    "maxLength": 1000,
    "pattern": "^[\\w\\s\\-\\[\\]\\(\\)\\{\\}\\.\\*\\+\\?\\|]+$"
  }
}
```

**属性说明**：
- `id`: 字段唯一标识符
- `type`: 控件类型，固定为 "text"
- `label`: 字段标签
- `description`: 字段描述
- `value`: 默认值
- `placeholder`: 占位符文本
- `required`: 是否必填
- `validation`: 验证规则（可选）
  - `minLength`: 最小长度
  - `maxLength`: 最大长度
  - `pattern`: 正则表达式验证

---

### 2. 滑块 (slider)

数值范围滑块控件。

**字段定义**：
```json
{
  "id": "threshold",
  "type": "slider",
  "label": "Threshold",
  "value": 0.5,
  "min": 0.0,
  "max": 1.0,
  "step": 0.01,
  "showValue": true
}
```

**属性说明**：
- `type`: 控件类型，固定为 "slider"
- `value`: 默认值（数值）
- `min`: 最小值
- `max`: 最大值
- `step`: 步长
- `showValue`: 是否显示当前值

---

### 3. 下拉框 (dropdown)

单选下拉列表控件。

**字段定义**：
```json
{
  "id": "language",
  "type": "dropdown",
  "label": "Language",
  "value": "cmn",
  "options": [
    {
      "value": "cmn",
      "label": "Mandarin Chinese"
    },
    {
      "value": "yue",
      "label": "Cantonese"
    },
    {
      "value": "jpn",
      "label": "Japanese"
    }
  ]
}
```

**属性说明**：
- `type`: 控件类型，固定为 "dropdown"
- `value`: 默认选中的值
- `options`: 选项列表
  - `value`: 选项值
  - `label`: 选项标签

---

### 4. SpinBox (spinbox)

整数数值输入控件。

**字段定义**：
```json
{
  "id": "batchSize",
  "type": "spinbox",
  "label": "Batch Size",
  "value": 32,
  "min": 1,
  "max": 256,
  "step": 1,
  "unit": "items"
}
```

**属性说明**：
- `type`: 控件类型，固定为 "spinbox"
- `value`: 默认值（整数）
- `min`: 最小值
- `max`: 最大值
- `step`: 步长
- `unit`: 单位

---

### 5. 单选按钮 (radio)

单选按钮组控件。

**字段定义**：
```json
{
  "id": "mode",
  "type": "radio",
  "label": "Mode",
  "value": "convert",
  "options": [
    {
      "value": "copy",
      "label": "Copy",
      "description": "Copy the original text"
    },
    {
      "value": "convert",
      "label": "Convert",
      "description": "Convert to phonetic notation"
    }
  ]
}
```

**属性说明**：
- `type`: 控件类型，固定为 "radio"
- `value`: 默认选中的值
- `options`: 选项列表
  - `value`: 选项值
  - `label`: 选项标签
  - `description`: 选项描述（可选）

---

### 6. 复选框 (checkbox)

布尔值复选框控件。

**字段定义**：
```json
{
  "id": "caseSensitive",
  "type": "checkbox",
  "label": "Case Sensitive",
  "value": false,
  "description": "Enable case-sensitive matching"
}
```

**属性说明**：
- `type`: 控件类型，固定为 "checkbox"
- `value`: 默认值（布尔值）
- `description`: 字段描述（可选）

---

## Section 结构

多个字段可以组织到一个 Section 中：

```json
{
  "id": "general",
  "title": "General Settings",
  "description": "Basic configuration options",
  "fields": [
    {...},
    {...}
  ]
}
```

**属性说明**：
- `id`: Section 唯一标识符
- `title`: Section 标题
- `description`: Section 描述（可选）
- `fields`: 字段列表

---

## 完整示例

### RegexSplitter UI Schema

```json
{
  "uiSchema": {
    "version": "1.0",
    "sections": [
      {
        "id": "general",
        "title": "General Settings",
        "fields": [
          {
            "id": "pattern",
            "type": "text",
            "label": "Pattern",
            "description": "Enter the regex pattern for splitting",
            "value": "([\\p{Han}])",
            "placeholder": "Enter pattern...",
            "required": true,
            "validation": {
              "minLength": 1,
              "maxLength": 1000,
              "pattern": "^[\\w\\s\\-\\[\\]\\(\\)\\{\\}\\.\\*\\+\\?\\|]+$"
            }
          },
          {
            "id": "caseSensitive",
            "type": "checkbox",
            "label": "Case Sensitive",
            "value": false,
            "description": "Enable case-sensitive matching"
          }
        ]
      },
      {
        "id": "advanced",
        "title": "Advanced Settings",
        "fields": [
          {
            "id": "batchSize",
            "type": "spinbox",
            "label": "Batch Size",
            "value": 100,
            "min": 1,
            "max": 1000,
            "step": 10,
            "unit": "texts"
          }
        ]
      }
    ]
  }
}
```

### MandarinG2p UI Schema

```json
{
  "uiSchema": {
    "version": "1.0",
    "sections": [
      {
        "id": "basic",
        "title": "Basic Settings",
        "fields": [
          {
            "id": "threshold",
            "type": "slider",
            "label": "Confidence Threshold",
            "value": 0.5,
            "min": 0.0,
            "max": 1.0,
            "step": 0.01,
            "showValue": true
          },
          {
            "id": "mode",
            "type": "radio",
            "label": "Conversion Mode",
            "value": "convert",
            "options": [
              {
                "value": "copy",
                "label": "Copy",
                "description": "Keep original text"
              },
              {
                "value": "convert",
                "label": "Convert",
                "description": "Convert to pinyin"
              }
            ]
          }
        ]
      },
      {
        "id": "advanced",
        "title": "Advanced Settings",
        "fields": [
          {
            "id": "fallbackStrategy",
            "type": "dropdown",
            "label": "Fallback Strategy",
            "value": "preserve",
            "options": [
              {
                "value": "preserve",
                "label": "Preserve Original"
              },
              {
                "value": "skip",
                "label": "Skip Unknown"
              },
              {
                "value": "empty",
                "label": "Return Empty"
              }
            ]
          },
          {
            "id": "debugMode",
            "type": "checkbox",
            "label": "Debug Mode",
            "value": false,
            "description": "Enable debug logging"
          }
        ]
      }
    ]
  }
}
```

---

## 字段属性汇总

### 通用属性

| 属性 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | string | ✅ | 字段唯一标识符 |
| `type` | enum | ✅ | 控件类型：`text`, `slider`, `dropdown`, `spinbox`, `radio`, `checkbox` |
| `label` | string | ✅ | 字段标签 |
| `description` | string | ❌ | 字段描述 |
| `value` | varies | ❌ | 默认值 |

### 特定控件属性

**text**：
- `placeholder`: string（占位符）
- `required`: boolean（是否必填）
- `validation`: ValidationRule（验证规则）

**slider**：
- `min`: number（最小值）
- `max`: number（最大值）
- `step`: number（步长）
- `showValue`: boolean（是否显示值）

**dropdown**：
- `options`: Option[]（选项列表）

**spinbox**：
- `min`: number（最小值）
- `max`: number（最大值）
- `step`: number（步长）
- `unit`: string（单位）

**radio**：
- `options`: RadioOption[]（选项列表）

**checkbox**：
- 无额外属性

---

## Schema 验证

推荐使用 JSON Schema 进行验证：

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["uiSchema"],
  "properties": {
    "uiSchema": {
      "type": "object",
      "required": ["version", "sections"],
      "properties": {
        "version": {
          "type": "string",
          "pattern": "^\\d+\\.\\d+$"
        },
        "sections": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["id", "title", "fields"],
            "properties": {
              "id": {
                "type": "string"
              },
              "title": {
                "type": "string"
              },
              "fields": {
                "type": "array"
              }
            }
          }
        }
      }
    }
  }
}
```

---

## 使用场景

### 1. 插件配置文件

每个插件可以在其包目录中包含 UI Schema 文件：

```
res/G2pPackages/Phonetic-Suite-Cmn/
├── package.json
├── modules/
│   └── G2p-Cmn/
│       ├── config.json
│       └── ui-schema.json        # UI Schema
```

### 2. 运行时加载

```cpp
// 伪代码示例
auto uiSchema = loadUiSchema(modulePath / "ui-schema.json");

// 生成 UI
for (const auto &section : uiSchema.sections) {
    createSection(section.title, section.description);
    
    for (const auto &field : section.fields) {
        auto widget = createWidget(field.type, field);
        setLabel(widget, field.label);
        setValue(widget, field.value);
    }
}
```

---

## 版本历史

- **1.0** (2026-04-03): 简化版本
  - 移除多语言本地化支持
  - 移除命名空间和继承机制
  - 简化验证规则
  - 聚焦核心功能和实际使用场景

---

**文档版本**：1.0
**创建日期**：2026-04-03
**作者**：Language Manager Team