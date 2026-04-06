# ChainG2p 设计文档

## 文档说明

本文档详细描述 ChainG2p 插件的设计，包括架构、组件、数据流、配置和扩展机制。

**版本**：1.1
**日期**：2026-04-04
**作者**：Language Manager 开发团队

---

## 目录

1. [概述](#概述)
2. [设计目标](#设计目标)
3. [架构设计](#架构设计)
4. [核心组件](#核心组件)
5. [内置处理步骤](#内置处理步骤)
6. [配置管理](#配置管理)
7. [扩展机制](#扩展机制)
8. [使用示例](#使用示例)

---

## 概述

ChainG2p 是一个基于责任链（Chain of Responsibility）模式的通用 G2p 处理框架，提供灵活、可扩展的文本到语音转换处理流程编排能力。

### 核心特性

- **步骤化处理**：将复杂的 G2p 处理流程分解为多个独立的步骤（Step）
- **灵活编排**：通过配置文件灵活组合和调整处理步骤的顺序
- **易于扩展**：添加新的处理步骤只需实现 G2pStep 接口并注册
- **策略丰富**：支持字典查找、模型推理、回退处理等多种处理策略
- **配置驱动**：通过 JSON 配置文件控制处理流程，无需重新编译

### 适用场景

- **多语言混合处理**：处理包含多种语言的文本
- **复杂处理流程**：需要多个处理阶段的文本转换
- **灵活策略选择**：根据不同情况选择不同的处理策略
- **可扩展需求**：需要添加自定义处理逻辑

### 与其他 G2p 插件的对比

| 特性 | ChainG2p | LstmG2p | MandarinG2p |
|------|----------|---------|-------------|
| 处理模式 | 责任链 | 单一模型 | 单一模型 |
| 灵活性 | 高 | 中 | 低 |
| 配置复杂度 | 高 | 中 | 低 |
| 性能 | 中 | 高 | 高 |
| 扩展性 | 高 | 低 | 低 |
| 适用场景 | 复杂流程 | 批量推理 | 标准转换 |

---

## 设计目标

### 1. 灵活性

**目标**：支持灵活的处理流程编排

**实现**：
- 通过配置文件定义处理步骤
- 支持动态调整步骤顺序
- 支持步骤的启用/禁用

### 2. 可扩展性

**目标**：易于添加新的处理步骤

**实现**：
- 定义清晰的 G2pStep 接口
- 提供步骤注册机制
- 支持自定义步骤参数

### 3. 可维护性

**目标**：每个步骤职责单一，易于维护

**实现**：
- 每个步骤独立实现
- 步骤之间通过 G2pContext 传递数据
- 提供清晰的错误处理机制

### 4. 性能优化

**目标**：在保证灵活性的前提下优化性能

**实现**：
- 支持批量处理
- 尽早过滤不需要处理的词
- 优先使用字典查找，避免不必要的模型推理

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────┐
│         ChainG2pTask                    │
│  (主任务类，管理整个处理流程)            │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│         G2pPipeline                     │
│  (处理管道，管理步骤的执行顺序)          │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│         G2pStep (抽象基类)              │
│  ┌─────────┬─────────┬─────────┐       │
│  │CleanStep│DictStep │ModelStep│ ...   │
│  └─────────┴─────────┴─────────┘       │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│         G2pContext                      │
│  (处理上下文，在步骤之间传递数据)        │
└─────────────────────────────────────────┘
```

---

## 核心组件

### 1. ChainG2pTask

**职责**：主任务类，管理整个处理流程

**主要接口**：
```cpp
class ChainG2pTask : public LangCore::Task {
public:
    explicit ChainG2pTask(const LangCore::ModuleSpec *spec);
    ~ChainG2pTask() override;

    int apiLevel() const override;
    LangCore::Expected<void> initialize() override;
    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    start(const LangCore::NO<LangCore::TaskInput> &input) override;

    std::string getConfig() const override;
    LangCore::Expected<void> setConfig(const std::string &config) override;

private:
    LangCore::VersionedTaskManager<ChainG2pTask> _manager;
};
```

### 2. G2pPipeline

**职责**：处理管道，管理步骤的执行顺序

**主要接口**：
```cpp
class G2pPipeline {
public:
    explicit G2pPipeline(const LangCore::ModuleSpec *spec);

    // 步骤管理
    LangCore::Expected<void> addStep(const std::string &stepType,
                                      const LangCore::JsonObject &config);
    void clearSteps();

    // 执行
    void execute(G2pContext &context);

    // 查询
    size_t stepCount() const;
    std::vector<std::string> stepNames() const;
};
```

### 3. G2pStep

**职责**：处理步骤基类，定义处理步骤的接口

**主要接口**：
```cpp
class G2pStep {
public:
    virtual ~G2pStep() = default;

    // 配置步骤
    virtual LangCore::Expected<void> configure(
        const LangCore::ModuleSpec *spec,
        const LangCore::JsonObject &config
    ) = 0;

    // 处理输入
    virtual void handle(G2pContext &context) = 0;

    // 清理资源
    virtual void cleanup() {}

    // 获取步骤名称
    virtual std::string name() const = 0;
};
```

### 4. G2pContext

**职责**：处理上下文，在步骤之间传递数据和状态

**数据结构**：
```cpp
class G2pContext {
public:
    // 单词信息
    struct WordInfo {
        std::string lyric;                    // 原词
        std::string cleanedLyric;             // 清洗后的词
        std::string tag;                      // 标记类型
        std::string language;                 // 语言类型
        bool discard = false;                 // 是否丢弃
        std::string mode;                     // 处理模式
        std::string pronunciation;            // 发音结果
        std::vector<std::string> candidates;  // 候选发音
        bool fromDict = false;                // 是否来自字典
        bool fromModel = false;               // 是否来自模型
        bool fromFallback = false;            // 是否来自回退
        std::map<std::string, std::any> metadata; // 元数据
    };

    // 访问方法
    std::vector<WordInfo>& words();
    const std::vector<WordInfo>& words() const;

    // 控制方法
    bool isStopProcessing() const;
    void setStopProcessing(bool stop);

    // 元数据访问
    void setMetadata(const std::string &key, const std::any &value);
    std::any getMetadata(const std::string &key) const;
};
```

### 5. G2pStepFactory

**职责**：步骤工厂，负责创建和注册处理步骤

**主要接口**：
```cpp
class G2pStepFactory {
public:
    // 创建步骤
    static LangCore::Expected<std::shared_ptr<G2pStep>> create(
        const std::string &stepType
    );

    // 获取所有支持的步骤类型
    static std::vector<std::string> supportedTypes();
};
```

**步骤注册宏**：
```cpp
#define REGISTER_STEP(StepClass, StepType) \
    namespace { \
        class StepClass##Registrar { \
        public: \
            StepClass##Registrar() { \
                G2pStepFactory::registerStep<StepClass>(StepType); \
            } \
        }; \
        static StepClass##Registrar g_##StepClass##Registrar; \
    }
```

---

## 内置处理步骤

### 1. CleanStep（清洗步骤）

**职责**：清洗输入文本，去除不需要的字符

**配置参数**：
```json
{
  "trim": true,                    // 去除首尾空白
  "lowercase": false,              // 转换为小写
  "uppercase": false,              // 转换为大写
  "removeDigits": false,           // 移除数字
  "removePunctuation": false,      // 移除标点符号
  "removeSymbols": false,          // 移除符号
  "customRegex": "",               // 自定义正则表达式
  "customReplace": ""              // 自定义替换字符串
}
```

### 2. TagAndValidateStep（标记和验证步骤）

**职责**：使用 Tag 插件进行语言标记，并根据标记设置处理模式

**配置参数**：
```json
{
  "language": "eng",               // 默认语言
  "tagger": [                      // 标记规则
    {
      "type": "regex",
      "value": ["^[A-Z]+$"],
      "tag": "uppercase",
      "action": "copy"
    },
    {
      "type": "regex",
      "value": ["^[0-9]+$"],
      "tag": "number",
      "action": "skip"
    }
  ],
  "enableStrictValidation": false  // 是否启用严格验证
}
```

### 3. DictStep（字典查找步骤）

**职责**：从 PhonemeDict 中查找发音

**配置参数**：
```json
{
  "enabled": true,                  // 是否启用
  "file": "assets/dict.txt",       // 字典文件路径
  "format": "word pronunciation",  // 字典格式
  "caseSensitive": false,           // 是否区分大小写
  "fallbackToOriginal": true        // 未找到时是否使用原词
}
```

### 4. ModelStep（模型推理步骤）

**职责**：使用 AI 模型生成发音

**配置参数**：
```json
{
  "enabled": true,                  // 是否启用
  "id": "g2p-lstm-eng",            // 模型 ID
  "batchSize": 50,                 // 批处理大小
  "confidenceThreshold": 0.5,       // 置信度阈值
  "fallbackOnLowConfidence": true   // 低置信度时是否回退
}
```

### 5. FallbackStep（回退处理步骤）

**职责**：提供回退策略，处理未转换的词

**配置参数**：
```json
{
  "strategy": "preserve",            // 回退策略：preserve, skip, empty, custom
  "customPronunciation": "",        // 自定义发音
  "markAsFallback": true            // 是否标记为回退
}
```

### 6. FormatStep（格式化步骤）

**职责**：格式化输出结果

**配置参数**：
```json
{
  "outputFormat": "standard",       // 输出格式：standard, detailed, custom
  "includeConfidence": false,      // 是否包含置信度
  "includeSource": false,          // 是否包含来源信息
  "customFormat": "{lyric} -> {pronunciation}"  // 自定义格式
}
```

---

## 配置管理

### 配置文件结构

```json
{
  "$version": "1.0",
  "level": 1,
  "steps": [
    {
      "type": "clean",
      "config": {
        "trim": true,
        "lowercase": true,
        "removePunctuation": true
      }
    },
    {
      "type": "tagAndValidate",
      "config": {
        "language": "eng",
        "tagger": [
          {
            "type": "regex",
            "value": ["^[A-Z]+$"],
            "tag": "uppercase",
            "action": "copy"
          }
        ]
      }
    },
    {
      "type": "dict",
      "config": {
        "enabled": true,
        "file": "assets/dict.txt"
      }
    },
    {
      "type": "model",
      "config": {
        "enabled": true,
        "id": "g2p-lstm-eng",
        "batchSize": 50
      }
    },
    {
      "type": "fallback",
      "config": {
        "strategy": "preserve"
      }
    },
    {
      "type": "format",
      "config": {
        "outputFormat": "standard"
      }
    }
  ]
}
```

### 配置验证

**验证规则**：
1. 必须包含 `steps` 数组
2. 每个步骤必须包含 `type` 字段
3. 步骤类型必须在支持的类型列表中
4. 步骤配置必须符合步骤的 schema

---

## 扩展机制

### 添加自定义步骤

**步骤 1**：创建步骤类

```cpp
class MyCustomStep : public G2pStep {
public:
    Expected<void> configure(const ModuleSpec *spec,
                            const JsonObject &config) override {
        // 配置步骤
        m_param1 = config.value("param1", "default");
        m_param2 = config.value("param2", 0);
        return {};
    }

    void handle(G2pContext &context) override {
        // 处理输入
        for (auto &word : context.words()) {
            if (word.discard) {
                continue;
            }
            // 自定义处理逻辑
            auto result = processWord(word.cleanedLyric);
            word.pronunciation = result.pronunciation;
            word.metadata["custom_data"] = result.metadata;
        }
    }

    void cleanup() override {
        // 清理资源
    }

    std::string name() const override {
        return "my-custom";
    }

private:
    std::string m_param1;
    int m_param2;
};
```

**步骤 2**：注册步骤

```cpp
REGISTER_STEP(MyCustomStep, "my-custom")
```

**步骤 3**：在配置中使用

```json
{
  "steps": [
    {
      "type": "my-custom",
      "config": {
        "param1": "value1",
        "param2": 100
      }
    }
  ]
}
```

---

## 使用示例

### 示例 1：基础使用

```cpp
#include <LangCore/Core/Manager.h>

int main() {
    // 初始化 Manager
    auto manager = LangCore::Manager::instance();
    std::string errMsg;
    if (!manager->initialize(errMsg)) {
        std::cerr << "Failed to initialize: " << errMsg << std::endl;
        return -1;
    }

    // 创建 ChainG2p 任务
    auto task = manager->task("g2p", "chain-g2p-eng");
    if (!task.ok()) {
        std::cerr << "Failed to create task" << std::endl;
        return -1;
    }

    // 准备输入
    std::vector<LangCore::G2pInput> inputs;
    inputs.emplace_back("hello", "chain-g2p-eng");
    inputs.emplace_back("WORLD", "chain-g2p-eng");

    // 执行转换
    auto results = manager->convert(inputs);

    // 输出结果
    for (const auto &result : results) {
        std::cout << result.lyric << " -> " << result.pronunciation << std::endl;
    }

    return 0;
}
```

### 示例 2：自定义配置

```cpp
// 设置自定义配置
std::string config = R"({
  "steps": [
    {
      "type": "clean",
      "config": {
        "trim": true,
        "lowercase": true
      }
    },
    {
      "type": "dict",
      "config": {
        "enabled": true,
        "file": "assets/custom-dict.txt"
      }
    }
  ]
})";

// 应用配置
auto setConfigResult = task.value()->setConfig(config);
```

### 示例 3：获取统计信息

```cpp
// 处理完成后获取统计信息
auto context = getProcessingContext();

std::cout << "Statistics:" << std::endl;
std::cout << "  Total words: " << context.words().size() << std::endl;
std::cout << "  Discarded: " << context.getDiscardCount() << std::endl;
std::cout << "  Copied: " << context.getCopyCount() << std::endl;
std::cout << "  Converted: " << context.getConvertCount() << std::endl;
```

---

**文档版本**: 1.1
**最后更新**: 2026-04-04
**更新内容**:
- 大幅精简文档，保留核心信息
- 简化代码示例和详细说明
- 保留所有关键概念和接口定义
- 优化文档结构，提高可读性