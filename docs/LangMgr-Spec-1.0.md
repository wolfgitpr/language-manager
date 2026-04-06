# Language Manager 数据格式与推理接口规范 1.0

> Language Manager Data Format and Inference Interface Specification 1.0

LangMgr 是一个模块化的语言处理插件系统，支持文本分割、语言标记、语音转换等任务。系统采用插件化设计，支持运行时动态加载和依赖管理。

## 1. 核心层级与组件

### 1.1 启动流程

```
PluginFactory->addPluginPath(const char *iid, const std::filesystem::path &path)
    ↓
PluginFactory->plugin(const char *iid, const char *key)
    ↓
PackageManager->open(package)
    ↓
PluginFactory->create()  (获取 DriverPlugin)
    ↓
SessionFactory->initialize(args)
    ↓
SessionFactory->createSession()
    ↓
SessionTask->open(path, args)
    ↓
Plugin->createTask(spec)  (获取 TaskPlugin)
    ↓
Task->start(input)
    ↓
TaskResult
```

### 1.2 插件系统

#### 1.2.1 Plugin 基础接口

Plugin 是系统扩展的基础单元，支持动态加载，通过唯一标识符（iid）和键值（key）注册功能组件。

#### 1.2.2 TaskPlugin

任务工厂插件，负责根据模块规范创建任务实例。

```cpp
class TaskPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Task"; }
    virtual Expected<NO<Task>> createTask(const ModuleSpec *spec) = 0;
};
```

#### 1.2.3 DriverPlugin

会话工厂插件，用于创建 AI 模型推理驱动，如 ONNX Driver，支持不同硬件后端（CPU、CUDA、DirectML 等）。

```cpp
class DriverPlugin : public Plugin {
public:
    const char *iid() const override { return "org.openvpi.Driver"; }
    virtual Expected<NO<SessionFactory>> create() = 0;
};
```

### 1.3 管理层级

#### 1.3.1 PluginFactory

插件管理器，负责插件的发现、加载和生命周期管理，维护插件路径和实例缓存。

#### 1.3.2 PackageManager

包管理系统，负责包的发现、加载、依赖解析和初始化顺序计算，维护包路径和模块元数据。

#### 1.3.3 Manager

提供高层 API（如文本分割、标注、G2P 转换），协调插件、包、任务的完整工作流程。

### 1.4 数据容器

#### 1.4.1 Package

模块的容器，包含多个模块及其相关资源文件。支持版本管理和依赖声明，通过 JSON 清单文件定义包的结构。

#### 1.4.2 ModuleSpec

模块的元数据和配置描述，包含模块的标识、类别、API 级别、配置信息等，是创建任务实例的蓝图。

#### 1.4.3 Task

实际执行处理逻辑的单元基类，定义任务的生命周期（初始化、启动）。SessionTask 为特殊类型，作为 AI 模型驱动的基类。

## 2. Package 格式规范

### 2.1 文件结构

本规范内，可分发的插件包的最小单位是 Package，是一个以 `lmpk` 为扩展名的 UTF-8 编码 ZIP 格式的压缩包。

压缩包内基本结构为：
```
+ xxx.lmpk
  - package.json
  - ...
```

Package 内多使用 `json` 作为声明文件，我们规定，声明文件中使用的相对路径的基路径是这个声明文件的所在目录。

### 2.2 package.json 描述文件

`package.json` 是 Package 的描述文件，主要包括以下内容。

```json
{
  "packageId": "eng-official",
  "version": "1.0.0",
  "vendor": {
    "_": "Vendor Name",
    "en": "Vendor Name",
    "zh": "供应商名称",
    "ja": "ベンダー名"
  },
  "copyright": {
    "_": "Copyright (C) Vendor",
    "en": "Copyright (C) Vendor",
    "zh": "版权所有 (C) 供应商",
    "ja": "著作権 (C) ベンダー"
  },
  "modules": {
    "tagger": [
      {
        "moduleId": "tagger-eng",
        "class": "tagger.template.RegexTaggerInference",
        "configuration": "modules/Tagger-Eng/config.json",
        "dependencies": []
      }
    ],
    "g2p": [
      {
        "moduleId": "g2p-lstm-eng",
        "class": "g2p.model.LstmG2pInference",
        "configuration": "modules/lstm-g2p-en/config.json"
      },
      {
        "moduleId": "g2p-eng",
        "class": "g2p.template.TemplateG2pInference",
        "configuration": "modules/template-eng/config.json",
        "dependencies": [
          {
            "packageId": "eng-official",
            "moduleId": "g2p-lstm-eng",
            "level": 1,
            "version": "*"
          }
        ]
      }
    ]
  }
}
```

#### 2.2.1 必选字段

- `packageId`：唯一标识符，禁止出现以下字符 `/\[]:;'"`

#### 2.2.2 可选字段

- `version`：版本号，格式为 `x.y.z`
- `vendor`：提供者信息，支持多语言
- `copyright`：版权信息，支持多语言
- `description`：介绍文字，支持多语言
- `readme`：放置介绍、许可证等信息的文本
- `url`：网站

#### 2.2.3 modules 字段

模块贡献列表，包含以下子模块：

**tagger（语言标记器）**：
- `moduleId`：模块 ID
- `class`：推理类型
- `configuration`：配置文件路径
- `dependencies`：依赖列表

**g2p（语音转换器）**：
- `moduleId`：模块 ID
- `class`：推理类型
- `configuration`：配置文件路径
- `dependencies`：依赖列表

**splitter（文本分割器）**：
- `moduleId`：模块 ID
- `class`：推理类型
- `configuration`：配置文件路径
- `dependencies`：依赖列表

### 2.3 依赖声明

依赖在模块的 `dependencies` 数组中声明：

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

**字段说明**：
- `packageId`：依赖包 ID
- `moduleId`：依赖模块 ID
- `level`：依赖 API 级别（用于兼容性检查）
- `version`：依赖版本（用于 Bug 修复检查）

## 3. 核心数据结构

### 3.1 G2pInput

```cpp
struct G2pInput {
    std::string lyric;    // 歌词/文本
    std::string g2pId;    // G2p ID
};
```

### 3.2 G2pRes

```cpp
struct G2pRes {
    std::string lyric;                          // 歌词/文本
    std::string g2pId;                          // G2p ID
    std::string pronunciation = lyric;          // 发音结果
    std::vector<std::string> candidates;        // 候选发音
    std::string mode = "copy";                  // 模式: "copy" 或 "convert"
};
```

### 3.3 TaggerRes

```cpp
struct TaggerRes {
    std::string lyric;              // 歌词/文本
    std::string language = "unknown"; // 语言 ID
    std::string tag = "unknown";    // 标签类型
    bool discard = false;           // 是否丢弃
};
```

## 4. API 接口规范

### 4.1 Manager 接口

```cpp
class Manager : public PackageManager {
public:
    static Manager *instance();
    bool initialize(std::string &errMsg);
    bool initialized() const;

    Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
    Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

    std::vector<std::string> split(const std::string &input);
    std::vector<TaggerRes> tag(const std::vector<std::string> &input, ...);
    std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);
};
```

### 4.2 Task 接口

```cpp
class Task : public NamedObject {
public:
    virtual int apiLevel() const = 0;
    virtual Expected<void> initialize() = 0;
    virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

    // 配置 API
    virtual std::string getConfig() const;
    virtual Expected<void> setConfig(const std::string &config);
    virtual std::string getUiSchema() const;
    virtual Expected<void> resetToDefault();
    virtual bool isUsingDefaultConfig() const;
};
```

### 4.3 SessionTask 接口

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

## 5. 配置管理规范

### 5.1 配置文件格式

配置文件使用 JSON 格式：

```json
{
  "$version": "1.0",
  "level": 1,
  "name": "My Module",
  "configuration": {
    "enabled": true,
    "param1": "value",
    "param2": 100
  }
}
```

### 5.2 配置持久化

Task 基类提供自动配置持久化功能：

1. **配置加载**：
   - 优先加载用户配置（`~/.config/language-manager/[taskId]/config.json`）
   - 回退到默认配置（`assets/config.json`）

2. **配置保存**：
   - 调用 `setConfig()` 时自动保存到用户配置目录
   - 支持配置热更新

## 6. 版本兼容性规范

### 6.1 Level 兼容性规则

**核心插件（Splitter/Tagger/G2p）**：

```
Manager Level = M
Plugin Level = P

兼容条件: M - 1 <= P <= M
```

| Manager Level | Plugin Level | 兼容性 |
|---------------|--------------|--------|
| 2 | 2 | ✅ 兼容 |
| 2 | 1 | ✅ 兼容 |
| 2 | 0 | ❌ 不兼容 |
| 2 | 3 | ❌ 不兼容 |

**依赖插件（Driver 等）**：
- 不对 Level 进行限制
- 通过 dependency 系统自动分析

### 6.2 Version 命名

- **Version**：语义化版本（MAJOR.MINOR.PATCH）
- **Version 的作用**：不参与 API 兼容性检查，仅用于 Bug 修复记录
- **推荐做法**：Version 第一位与 Level 相同（例如：Level=1, Version=1.2.3）

---

**文档版本**: 1.0
**最后更新**: 2026-04-04
**更新内容**:
- 规范化文档结构
- 完善核心组件说明
- 添加 API 接口规范
- 补充配置管理规范