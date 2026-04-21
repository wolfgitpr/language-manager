# DsDictTask - Dictionary Lookup Task

## 概述

DsDictTask 是一个高性能的词典查询任务，支持基于哈希的快速查询，避免重复读取词典文件。该任务可以作为依赖被其他使用词典查询结果的任务使用。

## 特性

- **哈希索引**：使用 `std::unordered_map` 实现快速查询
- **线程安全**：使用 `std::mutex` 保护词典访问
- **批量加载**：支持从文件批量加载词典条目
- **默认值支持**：查询失败时可返回默认值
- **元数据支持**：每个条目可附带元数据
- **词典哈希**：支持计算词典状态的哈希值，用于缓存验证

## API 设计

### DictInputV1

```cpp
class DictInputV1 : public TaskInput {
public:
    std::string dictId;      // 词典 ID
    std::string key;         // 要查询的键
    std::string defaultValue; // 可选默认值
    uint32_t flags = 0;      // 可选标志位
};
```

### DictResV1

```cpp
struct DictEntry {
    std::string key;       // 键
    std::string value;     // 值
    std::string metadata;  // 元数据（JSON 格式）
};

class DictResV1 : public TaskResult {
public:
    std::vector<DictEntry> entries;  // 查询结果条目
    bool found = false;              // 是否找到
    std::string errorMessage;        // 错误消息
};
```

### Dictionary 类（内部 API）

```cpp
class Dictionary {
public:
    // 添加单个条目
    void addEntry(const std::string& key, const std::string& value, const std::string& metadata = "");

    // 批量添加条目
    void addEntries(const std::vector<DictEntry>& entries);

    // 查询键
    bool lookup(const std::string& key, std::string& value, std::string& metadata) const;

    // 检查键是否存在
    bool contains(const std::string& key) const;

    // 获取所有条目
    std::vector<DictEntry> getAllEntries() const;

    // 清空词典
    void clear();

    // 获取词典 ID
    const std::string& id() const;

    // 获取条目数量
    size_t size() const;

    // 计算词典哈希
    size_t hash() const;
};
```

## 配置

### config.json

```json
{
  "$version": "1.0",
  "level": 1,
  "name": "DsDict Task",
  "description": "Dictionary lookup task with hash-based caching",
  "configuration": {
    "dictionaries": {
      "example-dict": "assets/example-dict.txt",
      "pinyin-dict": {
        "path": "assets/pinyin.txt",
        "encoding": "utf-8"
      }
    }
  }
}
```

### 词典文件格式

词典文件使用简单的制表符分隔格式：

```
# 注释行
键1	值1
键2	值2
键3	值3
```

## 使用示例

### 1. 初始化 Manager

```cpp
#include <LangCore/Runtime/Manager.h>

const auto langMgr = LangCore::Manager::instance();
std::string errorMessage;
if (!langMgr->initialize(errorMessage)) {
    std::cerr << "Failed to initialize: " << errorMessage << std::endl;
    return -1;
}
```

### 2. 查询词典

```cpp
// 创建查询输入
auto input = LangCore::NO<LangCore::DictInputV1>::create();
input->dictId = "example-dict";
input->key = "hello";
input->defaultValue = "未知";

// 获取词典任务
auto taskResult = langMgr->task("dict", "dsdict");
if (!taskResult) {
    auto err = taskResult.takeError();
    std::cerr << "Failed to get task: " << err.message() << std::endl;
    return -1;
}

auto task = taskResult.take();

// 执行查询
auto queryResult = task->start(input);
if (!queryResult) {
    auto err = queryResult.takeError();
    std::cerr << "Query failed: " << err.message() << std::endl;
    return -1;
}

auto result = LangCore::dynamic_pointer_cast<LangCore::DictResV1>(queryResult.take());

// 处理结果
if (result->found) {
    for (const auto& entry : result->entries) {
        std::cout << entry.key << " -> " << entry.value << std::endl;
        if (!entry.metadata.empty()) {
            std::cout << "  Metadata: " << entry.metadata << std::endl;
        }
    }
} else {
    std::cout << "Key not found" << std::endl;
}
```

### 3. 作为依赖使用

其他任务可以依赖 DsDictTask 来获取词典数据：

```cpp
// 在其他任务中查询词典
auto dictTask = Mgr()->getObject("dict", "dsdict");
if (dictTask.ok()) {
    auto input = LangCore::NO<LangCore::DictInputV1>::create();
    input->dictId = "pinyin-dict";
    input->key = "ni";

    auto result = dictTask.take()->start(input);
    if (result.ok()) {
        auto dictRes = LangCore::dynamic_pointer_cast<LangCore::DictResV1>(result.take());
        if (dictRes->found) {
            // 使用词典结果
            std::string pinyin = dictRes->entries[0].value;
        }
    }
}
```

## 高级用法

### 1. 动态添加词典

```cpp
// 通过配置加载词典后，可以通过内部 API 添加更多条目
// 注意：这需要在 DsDictTask 内部实现暴露 API

// 当前实现中，词典在初始化时加载
// 如需动态添加，建议重新实现 DsDictTask 或添加新的 API
```

### 2. 词典哈希验证

```cpp
// 获取词典哈希用于缓存验证
size_t dictHash = dict->hash();

// 如果哈希值相同，可以使用缓存的查询结果
if (cachedHash == dictHash) {
    return cachedResult;
}
```

### 3. 批量查询

```cpp
std::vector<std::string> keys = {"hello", "world", "good"};
std::vector<LangCore::DictEntry> results;

for (const auto& key : keys) {
    auto input = LangCore::NO<LangCore::DictInputV1>::create();
    input->dictId = "example-dict";
    input->key = key;

    auto result = task->start(input);
    if (result.ok()) {
        auto res = LangCore::dynamic_pointer_cast<LangCore::DictResV1>(result.take());
        if (res->found) {
            results.push_back(res->entries[0]);
        }
    }
}
```

## 性能优化

1. **哈希索引**：使用 `std::unordered_map` 实现 O(1) 平均查询复杂度
2. **避免重复加载**：词典加载后常驻内存，避免重复读取文件
3. **线程安全**：使用细粒度锁保护词典访问，支持并发查询
4. **哈希缓存**：支持计算词典哈希，可用于验证缓存有效性

## 文件结构

```
plugins/Dicts/DsDict/
├── CMakeLists.txt
├── main.cpp
├── DsDictTask.h
├── DsDictTask.cpp
├── assets/
│   ├── config.json
│   └── example-dict.txt
└── internal/
    └── V1/
        ├── TaskImpl.h
        └── TaskImpl.cpp
```

## 依赖关系

- LangCore::LangCore
- C++17 标准库
- stdcorelib

## 插件信息

- **插件 ID**: dict.dsdict
- **API Level**: 1
- **类别**: dict
- **用途**: 词典查询和缓存

## 注意事项

1. 词典文件使用 UTF-8 编码
2. 每行一个条目，使用制表符（\t）分隔键和值
3. 以 # 开头的行被视为注释
4. 空行会被忽略
5. 查询是大小写敏感的

## 未来扩展

1. 支持更多词典格式（JSON、SQLite 等）
2. 支持模糊查询和正则表达式匹配
3. 支持词典热重载
4. 支持分布式词典存储
5. 支持词典版本管理和更新