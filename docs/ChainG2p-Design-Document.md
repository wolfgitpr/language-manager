# ChainG2p 设计文档

**版本**：3.0  
**日期**：2026-04-27

---

## 1. 概述

ChainG2p 是一个基于责任链模式的 G2p 处理框架。将 G2p 流程分解为多个独立步骤（Step），通过 JSON 配置文件编排执行顺序。

**适用场景**：需要多阶段处理的语言（如英语：先查字典 → 未命中的用模型推理 → 兜底回退），或纯规则处理的语言（如标点、数字）。

---

## 2. 架构

```
ChainG2pTask
  └── VersionedTaskManager → ChainG2pTaskImpl (V1)
        └── G2pPipeline
              └── vector<G2pStep>
                    ├── TagAndValidateStep
                    ├── DictStep
                    ├── ModelStep
                    ├── FallbackStep
                    └── FormatStep
```

数据通过 `G2pContext`（含 `vector<WordInfo>`）在步骤间传递。

---

## 3. 核心组件

### G2pContext

处理上下文，在步骤间传递数据：

```cpp
struct WordInfo {
    // 原始信息
    std::string lyric;           // 原词
    std::string cleanedLyric;    // 清洗后的词

    // 标记信息
    std::string tag;             // 标记类型
    std::string language;        // 语言类型
    bool discard = false;        // 是否丢弃

    // 处理模式
    std::string mode;            // "copy" | "convert" | "skip"

    // 结果信息
    std::string pronunciation;   // 发音结果
    std::vector<std::string> candidates;  // 候选发音
    G2pErrorType errorType = NoError;     // 错误类型

    // 来源标记
    bool fromDict = false;       // 是否来自字典
    bool fromModel = false;      // 是否来自模型
    bool fromFallback = false;   // 是否来自回退
};
```

`G2pContext` 构造时接收输入字符串数组和 `ModuleSpec` 指针，提供 `words()` 访问词列表，`spec()` / `mgr()` 访问模块信息，`isStopProcessing()` / `setStopProcessing()` 控制流水线提前中断。

### G2pPipeline

管道编排器。`configure()` 解析 JSON 中的 `"steps"` 数组，通过 `G2pStepFactory::create()` 创建步骤实例。`process()` 依次执行各步骤，支持 `context.isStopProcessing()` 提前中断。

### G2pStep

步骤基类：

```cpp
class G2pStep {
public:
    virtual Expected<void> configure(const ModuleSpec *spec,
                                     const JsonObject &config) = 0;
    virtual void handle(G2pContext &context) = 0;
    virtual void cleanup() {}
    virtual std::string name() const = 0;

    void setTask(Task* task);  // 设置关联 Task（用于访问其他 Task）

protected:
    const ModuleSpec* m_spec = nullptr;
    PackageManager* m_mgr = nullptr;
    Task* m_task = nullptr;
    std::string m_config;
};
```

### G2pStepFactory

简单工厂，硬编码 5 种步骤类型的创建逻辑：

| 类型字符串 | 对应类 |
|-----------|--------|
| `"tagAndValidate"` | `TagAndValidateStep` |
| `"dict"` | `DictStep` |
| `"model"` | `ModelStep` |
| `"format"` | `FormatStep` |
| `"fallback"` | `FallbackStep` |

`supportedTypes()` 返回以上 5 种类型名。未知类型返回 `ConfigError`。

---

## 4. 内置步骤

### TagAndValidateStep

根据配置规则对每个词设置 `mode`（copy / convert）。规则类型：

| 类型 | 配置字段 | 行为 |
|------|---------|------|
| regex | `"value": ["pattern"]` | RE2 全匹配 |
| stopword | `"value": ["word1", "word2"]` | 集合精确匹配 |

配置示例：
```json
{
  "step": "tagAndValidate",
  "params": {
    "tagger": [
      { "type": "regex", "value": ["^[A-Z]+$"], "tag": "uppercase", "action": "copy" },
      { "type": "regex", "value": ["^[a-z]+$"], "tag": "lowercase", "action": "convert" }
    ]
  }
}
```

### DictStep

从 PhonemeDict 文件查找发音。仅处理 `mode == "convert"` 的词。

```json
{
  "step": "dict",
  "params": {
    "enabled": true,
    "file": "assets/dict.txt"
  }
}
```

### ModelStep

通过 ONNX 子任务推理。按 `batchSize` 批量处理未转换的词。如果 ONNX 任务不可用，优雅降级（保留原词，设置 `errorType = DriverUnavailable`）。

```json
{
  "step": "model",
  "params": {
    "enabled": true,
    "id": "g2p-lstm-eng-official",
    "batchSize": 50
  }
}
```

### FallbackStep

兜底处理所有仍未获得发音的词。

```json
{
  "step": "fallback",
  "params": {
    "useOriginal": true,
    "defaultPronunciation": "",
    "markFailed": true
  }
}
```

### FormatStep

后处理：去除尾部空格、音素间加空格、音调归一化。

```json
{
  "step": "format",
  "params": {
    "stripTrailingSpace": true,
    "addSpaceBetweenPhones": true,
    "normalizeTones": false
  }
}
```

---

## 5. 配置文件完整示例

```json
{
  "$version": "1.0",
  "level": 1,
  "mode": "standard",
  "steps": [
    {
      "step": "tagAndValidate",
      "params": {
        "tagger": [
          { "type": "regex", "value": ["^[A-Z]+$"], "tag": "uppercase", "action": "copy" },
          { "type": "regex", "value": ["^[a-z]+$"], "tag": "lowercase", "action": "convert" }
        ]
      }
    },
    { "step": "dict", "params": { "file": "ds_cmudict-07b.txt" } },
    { "step": "model", "params": { "id": "g2p-lstm-eng-official", "batchSize": 50 } },
    { "step": "fallback", "params": {} },
    { "step": "format", "params": {} }
  ]
}
```

---

## 6. 线程安全

`ChainG2pTaskImpl` 使用 `shared_mutex`：`initialize()` 持写锁，`getConfig()` 持读锁。`start()` 不持锁——每次调用创建独立的 `G2pContext`，无共享可变状态。Pipeline 和 Step 本身不保证线程安全——并发由 TaskImpl 层控制。

> **注意**：当前 `start()` 无锁设计假设 Pipeline 在 `initialize()` 后不可变。若未来支持运行时重新配置 Pipeline，需要在 `start()` 中增加读锁。

---

## 7. 扩展新步骤

1. 创建步骤类，继承 `G2pStep`，实现 `configure()` / `handle()` / `name()`
2. 在 `G2pStepFactory::create()` 中添加分支
3. 在 `G2pStepFactory::supportedTypes()` 中添加类型名

---

**文档版本**: 3.0  
**最后更新**: 2026-04-27
