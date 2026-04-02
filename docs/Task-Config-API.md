# Task配置API设计

## 1. 概述

本文档描述了Task的配置API设计，允许动态获取和设置Task的配置，以JSON格式传参，这样可以忽略实现细节，便于多版本使用同一个接口。

**重要说明**：
- 当前 Task 配置 API 仅包含两个方法：`getConfig()` 和 `setConfig()`
- `getConfig()` 返回类型为 `std::string`，不使用 `Expected<T>`
- `setConfig()` 返回类型为 `Expected<void>`，用于错误处理
- 配置更新由插件自行实现，Task 基类只提供基础的 get/set 接口
- 文档中提到的 `updateConfig()`、`getConfigValue()` 和 `setConfigValue()` 方法在实际代码中不存在

## 2. 设计目标

1. **统一接口**：所有Task都支持相同的配置API
2. **JSON格式**：使用JSON作为配置格式，易于理解和处理
3. **动态配置**：支持运行时动态修改配置
4. **版本兼容**：配置接口版本无关，支持多版本Task
5. **类型安全**：配置验证和类型检查

## 3. API定义

### 实际可用的配置 API

当前 Task 配置 API 仅包含以下两个方法：

1. **`getConfig()`** - 获取完整配置（JSON 字符串）
   - 返回类型：`std::string`
   - 不使用 `Expected<T>` 返回类型
   - 失败时返回空字符串

2. **`setConfig()`** - 设置完整配置（JSON 字符串）
   - 返回类型：`Expected<void>`
   - 使用 `Expected<T>` 返回类型进行错误处理
   - 需要传入完整的配置 JSON

**重要提示**：
- 不支持部分配置更新
- 不支持单个配置项的获取和设置
- 配置更新由插件自行实现，Task 基类只提供基础的 get/set 接口

### 3.1 getConfig()

**功能**：获取当前配置（JSON格式）

**签名**：
```cpp
virtual std::string getConfig() const;
```

**返回值**：
- 成功：返回JSON格式的配置字符串
- 失败：返回空字符串

**说明**：
- 此方法不使用 `Expected<T>` 返回类型，直接返回 `std::string`
- 配置格式为 JSON 字符串
- 如果配置获取失败，返回空字符串

**示例**：
```cpp
std::string config = task->getConfig();
if (!config.empty()) {
    std::cout << config << std::endl;
    // 输出：{"enabled":true,"param1":"value1","param2":100}
}
```

### 3.2 setConfig()

**功能**：设置配置（JSON格式）

**签名**：
```cpp
virtual Expected<void> setConfig(const std::string &config);
```

**参数**：
- config：JSON格式的配置字符串

**返回值**：
- 成功：返回Expected<void>
- 失败：返回Error

**示例**：
```cpp
std::string config = R"({
    "enabled": true,
    "param1": "value1",
    "param2": 100
})";

auto result = task->setConfig(config);
if (!result) {
    std::cerr << "Failed to set config: " << result.error().message() << std::endl;
}
```

### 3.3 updateConfig()

⚠️ **警告**：此方法在实际代码中不存在，以下内容仅供参考或未来实现。

**功能**：更新部分配置（JSON格式）

**签名**：
```cpp
virtual Expected<void> updateConfig(const std::string &config);
```

**参数**：
- config：JSON格式的配置字符串（只包含需要更新的字段）

**返回值**：
- 成功：返回Expected<void>
- 失败：返回Error

**示例**：
```cpp
// 只更新param2的值
std::string config = R"({
    "param2": 200
})";

auto result = task->updateConfig(config);
if (!result) {
    std::cerr << "Failed to update config: " << result.error().message() << std::endl;
}
```

### 3.4 getConfigValue()

⚠️ **警告**：此方法在实际代码中不存在，以下内容仅供参考或未来实现。

**功能**：获取配置值

**签名**：
```cpp
virtual Expected<std::string> getConfigValue(const std::string &key) const;
```

**参数**：
- key：配置键（支持点号分隔的路径，如"processor.enabled"）

**返回值**：
- 成功：返回配置值（JSON格式字符串）
- 失败：返回Error

**示例**：
```cpp
auto enabled = task->getConfigValue("enabled");
if (enabled) {
    std::cout << "enabled: " << enabled.value() << std::endl;
    // 输出：enabled: true
}

auto param2 = task->getConfigValue("param2");
if (param2) {
    std::cout << "param2: " << param2.value() << std::endl;
    // 输出：param2: 100
}
```

### 3.5 setConfigValue()

⚠️ **警告**：此方法在实际代码中不存在，以下内容仅供参考或未来实现。

**功能**：设置配置值

**签名**：
```cpp
virtual Expected<void> setConfigValue(const std::string &key, const std::string &value);
```

**参数**：
- key：配置键（支持点号分隔的路径，如"processor.enabled"）
- value：配置值（JSON格式字符串）

**返回值**：
- 成功：返回Expected<void>
- 失败：返回Error

**示例**：
```cpp
auto result = task->setConfigValue("enabled", "false");
if (!result) {
    std::cerr << "Failed to set config value: " << result.error().message() << std::endl;
}

result = task->setConfigValue("param2", "300");
if (!result) {
    std::cerr << "Failed to set config value: " << result.error().message() << std::endl;
}
```

## 4. 配置格式

### 4.1 基本配置

```json
{
    "enabled": true,
    "param1": "value1",
    "param2": 100,
    "param3": [1, 2, 3],
    "param4": {
        "subParam1": "subValue1",
        "subParam2": 200
    }
}
```

### 4.2 OnnxDriver配置

```json
{
    "modelPath": "/path/to/model.onnx",
    "device": "cpu",
    "providers": ["CPUExecutionProvider"],
    "enableDirectML": false,
    "enableCUDA": true,
    "batchSize": 1,
    "numThreads": 4
}
```

## 5. 实现示例

⚠️ **警告**：以下实现示例中使用的方法（如 `updateConfig()`、`getConfigValue()`、`setConfigValue()`）在实际代码中不存在。这些示例仅供参考或用于未来实现规划。

### 5.1 基本实现

```cpp
class MyTask : public Task {
public:
    MyTask(const ModuleSpec *spec) : Task(spec) {}

    Expected<std::string> getConfig() const override {
        // 序列化配置为JSON
        return serializeConfig();
    }

    Expected<void> setConfig(const std::string &config) override {
        // 解析配置
        auto result = parseConfig(config);
        if (!result) {
            return result.error();
        }

        // 验证配置
        auto validateResult = validateConfig(result.value());
        if (!validateResult) {
            return validateResult.error();
        }

        // 应用配置
        m_config = result.value();
        return {};
    }

    Expected<void> updateConfig(const std::string &config) override {
        // 解析部分配置
        auto result = parseConfig(config);
        if (!result) {
            return result.error();
        }

        // 合并配置
        mergeConfig(m_config, result.value());
        return {};
    }

    Expected<std::string> getConfigValue(const std::string &key) const override {
        // 查找配置值
        return findConfigValue(m_config, key);
    }

    Expected<void> setConfigValue(const std::string &key, const std::string &value) override {
        // 解析值
        auto parsedValue = parseJson(value);
        if (!parsedValue) {
            return parsedValue.error();
        }

        // 设置配置值
        return setConfigValue(m_config, key, parsedValue.value());
    }

private:
    nlohmann::json m_config;

    Expected<std::string> serializeConfig() const;
    Expected<nlohmann::json> parseConfig(const std::string &config) const;
    Expected<void> validateConfig(const nlohmann::json &config) const;
    void mergeConfig(nlohmann::json &target, const nlohmann::json &source);
    Expected<std::string> findConfigValue(const nlohmann::json &config, const std::string &key) const;
    Expected<void> setConfigValue(nlohmann::json &config, const std::string &key, const nlohmann::json &value);
};
```

### 5.2 使用nlohmann/json

```cpp
Expected<std::string> MyTask::serializeConfig() const {
    try {
        return m_config.dump(2);  // 格式化输出
    } catch (const std::exception &e) {
        return Error(Error::RuntimeError, stdc::formatN("Failed to serialize config: %1", e.what()));
    }
}

Expected<nlohmann::json> MyTask::parseConfig(const std::string &config) const {
    try {
        return nlohmann::json::parse(config);
    } catch (const std::exception &e) {
        return Error(Error::InvalidFormat, stdc::formatN("Failed to parse config: %1", e.what()));
    }
}

Expected<void> MyTask::validateConfig(const nlohmann::json &config) const {
    // 验证必需字段
    if (!config.contains("enabled")) {
        return Error(Error::InvalidFormat, "Missing required field: enabled");
    }

    if (!config["enabled"].is_boolean()) {
        return Error(Error::InvalidFormat, "Field 'enabled' must be boolean");
    }

    return {};
}

void MyTask::mergeConfig(nlohmann::json &target, const nlohmann::json &source) {
    // 递归合并配置
    for (auto it = source.begin(); it != source.end(); ++it) {
        if (target.contains(it.key()) && target[it.key()].is_object() && it.value().is_object()) {
            mergeConfig(target[it.key()], it.value());
        } else {
            target[it.key()] = it.value();
        }
    }
}

Expected<std::string> MyTask::findConfigValue(const nlohmann::json &config, const std::string &key) const {
    try {
        // 支持点号分隔的路径
        auto keys = stdc::split(key, ".");
        nlohmann::json current = config;

        for (const auto &k : keys) {
            if (!current.contains(k)) {
                return Error(Error::InvalidArgument, stdc::formatN("Config key not found: %1", key));
            }
            current = current[k];
        }

        return current.dump();
    } catch (const std::exception &e) {
        return Error(Error::RuntimeError, stdc::formatN("Failed to find config value: %1", e.what()));
    }
}

Expected<void> MyTask::setConfigValue(nlohmann::json &config, const std::string &key, const nlohmann::json &value) {
    try {
        // 支持点号分隔的路径
        auto keys = stdc::split(key, ".");
        nlohmann::json *current = &config;

        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (!current->contains(keys[i])) {
                (*current)[keys[i]] = nlohmann::json::object();
            }
            current = &(*current)[keys[i]];
        }

        (*current)[keys.back()] = value;
        return {};
    } catch (const std::exception &e) {
        return Error(Error::RuntimeError, stdc::formatN("Failed to set config value: %1", e.what()));
    }
}
```

## 6. 使用场景

⚠️ **警告**：以下使用场景示例中使用的方法（如 `setConfigValue()`）在实际代码中不存在。这些示例仅供参考或用于未来实现规划。

### 6.1 动态调整参数

```cpp
// 根据用户输入调整参数
task->setConfigValue("batchSize", "8");
task->setConfigValue("numThreads", "8");
```

### 6.2 启用/禁用功能

```cpp
// 启用缓存
task->setConfigValue("enableCache", "true");

// 禁用调试模式
task->setConfigValue("enableDebug", "false");
```

### 6.3 切换处理器

```cpp
// 禁用字典查找，强制使用推理引擎
task->setConfigValue("processors[1].enabled", "false");
```

### 6.4 配置验证

```cpp
// 获取配置进行验证
auto config = task->getConfig();
if (config) {
    auto json = nlohmann::json::parse(config.value());
    // 验证配置
    validateConfiguration(json);
}
```

### 6.5 配置持久化

```cpp
// 保存配置到文件
auto config = task->getConfig();
if (config) {
    std::ofstream file("config.json");
    file << config.value();
}

// 从文件加载配置
std::ifstream file("config.json");
std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
task->setConfig(content);
```

## 7. 错误处理

### 7.1 错误类型

| 错误类型 | 说明 | 处理建议 |
|---------|------|---------|
| InvalidFormat | 配置格式无效 | 检查JSON语法 |
| InvalidArgument | 参数无效 | 检查配置键是否存在 |
| Runtime | 运行时错误 | 检查配置值是否合法 |
| NotImplemented | 功能未实现 | 检查Task是否支持该功能 |

### 7.2 错误处理示例

```cpp
auto result = task->setConfig(config);
if (!result) {
    auto error = result.error();
    switch (error.type()) {
        case Error::InvalidFormat:
            std::cerr << "Invalid config format: " << error.message() << std::endl;
            break;
        case Error::InvalidArgument:
            std::cerr << "Invalid argument: " << error.message() << std::endl;
            break;
        default:
            std::cerr << "Error: " << error.message() << std::endl;
            break;
    }
}
```

## 8. 最佳实践

### 8.1 配置验证

```cpp
Expected<void> MyTask::validateConfig(const nlohmann::json &config) const {
    // 验证必需字段
    if (!config.contains("requiredField")) {
        return Error(Error::InvalidFormat, "Missing required field: requiredField");
    }

    // 验证字段类型
    if (!config["requiredField"].is_string()) {
        return Error(Error::InvalidFormat, "Field 'requiredField' must be string");
    }

    // 验证字段值范围
    if (config.contains("count")) {
        int count = config["count"].get<int>();
        if (count < 0 || count > 100) {
            return Error(Error::InvalidArgument, "Field 'count' must be between 0 and 100");
        }
    }

    return {};
}
```

### 8.2 默认配置

```cpp
MyTask::MyTask(const ModuleSpec *spec) : Task(spec) {
    // 设置默认配置
    m_config = {
        {"enabled", true},
        {"param1", "default1"},
        {"param2", 100},
        {"enableCache", true},
        {"enableDebug", false}
    };
}
```

### 8.3 配置版本控制

```json
{
    "configVersion": "1.0",
    "enabled": true,
    "param1": "value1",
    "param2": 100
}
```

```cpp
Expected<void> MyTask::validateConfig(const nlohmann::json &config) const {
    // 检查配置版本
    if (config.contains("configVersion")) {
        std::string version = config["configVersion"].get<std::string>();
        if (version != "1.0") {
            return Error(Error::InvalidArgument, stdc::formatN("Unsupported config version: %1", version));
        }
    }

    // ... 其他验证
    return {};
}
```

### 8.4 配置热更新

```cpp
// 监听配置文件变化
void watchConfigFile(const std::string &path, Task *task) {
    std::filesystem::path configPath(path);
    std::filesystem::file_time_type lastWriteTime;

    while (true) {
        try {
            auto currentWriteTime = std::filesystem::last_write_time(configPath);
            if (currentWriteTime != lastWriteTime) {
                lastWriteTime = currentWriteTime;

                // 加载新配置
                std::ifstream file(path);
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

                auto result = task->setConfig(content);
                if (!result) {
                    std::cerr << "Failed to update config: " << result.error().message() << std::endl;
                } else {
                    std::cout << "Config updated successfully" << std::endl;
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error watching config file: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

## 9. 性能考虑

### 9.1 配置缓存

```cpp
class MyTask : public Task {
public:
    Expected<std::string> getConfig() const override {
        // 缓存配置序列化结果
        if (m_configCache.empty()) {
            auto result = serializeConfig();
            if (!result) {
                return result.error();
            }
            m_configCache = result.value();
        }
        return m_configCache;
    }

    Expected<void> setConfig(const std::string &config) override {
        auto result = parseConfig(config);
        if (!result) {
            return result.error();
        }

        m_config = result.value();
        m_configCache.clear();  // 清除缓存
        return {};
    }

private:
    mutable std::string m_configCache;
};
```

### 9.2 延迟解析

```cpp
class MyTask : public Task {
public:
    Expected<void> setConfig(const std::string &config) override {
        // 延迟解析，只存储字符串
        m_configStr = config;
        m_configDirty = true;
        return {};
    }

    Expected<void> initialize() override {
        // 在初始化时解析配置
        if (m_configDirty) {
            auto result = parseConfig(m_configStr);
            if (!result) {
                return result.error();
            }

            auto validateResult = validateConfig(result.value());
            if (!validateResult) {
                return validateResult.error();
            }

            m_config = result.value();
            m_configDirty = false;
        }
        return {};
    }

private:
    std::string m_configStr;
    bool m_configDirty = false;
    nlohmann::json m_config;
};
```

## 10. 测试

⚠️ **警告**：以下测试用例中使用的方法（如 `updateConfig()`、`getConfigValue()`）在实际代码中不存在。这些示例仅供参考或用于未来实现规划。

### 10.1 单元测试

```cpp
TEST(TaskConfigTest, SetAndGetConfig) {
    auto task = createTask();

    std::string config = R"({"enabled":true,"param1":"value1"})";
    auto result = task->setConfig(config);
    EXPECT_TRUE(result.ok());

    std::string getConfig = task->getConfig();
    EXPECT_FALSE(getConfig.empty());
    EXPECT_EQ(getConfig, config);
}

TEST(TaskConfigTest, UpdateConfig) {
    auto task = createTask();

    std::string config = R"({"enabled":true,"param1":"value1","param2":100})";
    task->setConfig(config);

    std::string update = R"({"param2":200})";
    auto result = task->updateConfig(update);
    EXPECT_TRUE(result.ok());

    auto param2 = task->getConfigValue("param2");
    EXPECT_TRUE(param2.ok());
    EXPECT_EQ(param2.value(), "200");
}

TEST(TaskConfigTest, GetConfigValue) {
    auto task = createTask();

    std::string config = R"({"enabled":true,"param1":"value1"})";
    task->setConfig(config);

    auto enabled = task->getConfigValue("enabled");
    EXPECT_TRUE(enabled.ok());
    EXPECT_EQ(enabled.value(), "true");
}
```

### 10.2 配置验证测试

```cpp
TEST(TaskConfigTest, InvalidConfig) {
    auto task = createTask();

    std::string invalidConfig = R"({"enabled":"not_a_boolean"})";
    auto result = task->setConfig(invalidConfig);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().type(), Error::InvalidFormat);
}
```

## 11. API 局限性与未来改进

### 11.1 当前限制

当前 Task 配置 API 存在以下限制：

1. **API 方法有限**：
   - 仅提供 `getConfig()` 和 `setConfig()` 两个方法
   - 不支持部分配置更新（如 `updateConfig()`）
   - 不支持单个配置项的获取和设置（如 `getConfigValue()`、`setConfigValue()`）

2. **错误处理不一致**：
   - `getConfig()` 返回 `std::string`，无法返回详细的错误信息
   - `setConfig()` 返回 `Expected<void>`，可以返回详细的错误信息
   - 这种不一致可能导致开发者困惑

3. **配置验证依赖插件**：
   - 配置验证完全由插件实现
   - Task 基类不提供配置验证框架
   - 不同插件的配置验证行为可能不一致

### 11.2 未来改进方向

为了改进配置 API 的易用性和功能性，可以考虑以下改进：

1. **扩展 API 方法**：
   ```cpp
   // 未来可能添加的方法
   virtual Expected<void> updateConfig(const std::string &config);
   virtual Expected<std::string> getConfigValue(const std::string &key) const;
   virtual Expected<void> setConfigValue(const std::string &key, const std::string &value);
   ```

2. **统一错误处理**：
   ```cpp
   // 改进 getConfig() 方法，使其也返回 Expected<T>
   virtual Expected<std::string> getConfig() const;
   ```

3. **添加配置验证框架**：
   - 在 Task 基类中提供配置验证接口
   - 定义标准的配置验证规则
   - 提供配置验证错误的标准格式

4. **配置变更通知**：
   - 添加配置变更回调机制
   - 允许插件监听配置变更事件
   - 支持配置变更前的验证和确认

### 11.3 迁移指南

如果未来 API 发生变化，需要考虑向后兼容性：

1. **保留现有 API**：
   - 保持 `getConfig()` 和 `setConfig()` 方法的签名不变
   - 通过继承或扩展来添加新功能

2. **提供迁移工具**：
   - 提供配置格式转换工具
   - 自动检测和迁移旧配置格式
   - 提供迁移文档和示例

3. **版本标记**：
   - 在配置 JSON 中添加版本字段
   - 根据版本号使用不同的配置处理逻辑
   - 提供版本升级和降级支持

## 12. 总结

Task配置API提供了统一、灵活、类型安全的配置管理机制：

1. **统一接口**：所有Task都支持相同的配置API
2. **JSON格式**：使用JSON作为配置格式，易于理解和处理
3. **动态配置**：支持运行时动态修改配置
4. **版本兼容**：配置接口版本无关，支持多版本Task
5. **类型安全**：配置验证和类型检查

通过这个API，开发者可以：
- 动态调整Task参数
- 启用/禁用功能
- 热更新配置
- 配置持久化

**注意事项**：
- 当前 API 仅包含 `getConfig()` 和 `setConfig()` 两个方法
- 文档中提到的其他方法（如 `updateConfig()`、`getConfigValue()`、`setConfigValue()`）在实际代码中不存在
- 使用 `setConfig()` 时需要传入完整的配置 JSON，不支持部分更新
- `getConfig()` 失败时返回空字符串，需要根据业务逻辑判断是否有效
- 统一的配置管理