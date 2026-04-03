# Config 持久化设计方案

## 1. 设计概述

本设计方案为 Language Manager 插件系统提供完整的配置持久化方案，确保用户配置能够安全存储、加载和恢复，同时不破坏原有的 package 配置文件结构。

### 1.1 设计目标

1. **向后兼容**：不破坏原有的 package.json 和 modules/ 目录结构
2. **按模块存储**：每个 module 的配置独立存储，便于管理
3. **默认配置分离**：区分默认配置和用户自定义配置
4. **灵活恢复**：支持恢复默认配置和配置版本管理
5. **类型安全**：基于 Task 接口的配置管理，保持一致性

### 1.2 核心原则

1. **默认配置不变**：modules/ 目录中的配置文件保持不变，作为只读默认配置
2. **用户配置独立**：configs/ 目录存储用户自定义配置，可读可写
3. **自动合并**：加载时自动合并默认配置和用户配置
4. **原子操作**：配置保存采用原子操作，避免数据损坏

---

## 2. 存储结构设计

### 2.1 Package 目录结构

```
Package.lmpk
├── package.json                    # 包元数据（只读）
├── modules/                       # 默认配置目录（只读）
│   ├── Splitter-Cmn/
│   │   └── config.json           # splitter 默认配置
│   ├── Tagger-Cmn/
│   │   └── config.json           # tagger 默认配置
│   └── G2p-Cmn/
│       └── config.json           # g2p 默认配置
├── configs/                       # 用户配置目录（读写）
│   ├── splitter-cmn.json          # splitter 用户配置
│   ├── tagger-cmn.json           # tagger 用户配置
│   └── g2p-cmn.json              # g2p 用户配置
├── config-backup/                 # 配置备份目录（可选）
│   ├── splitter-cmn.json.backup
│   ├── tagger-cmn.json.backup
│   └── g2p-cmn.json.backup
└── assets/                        # 资源文件（只读）
    └── ...
```

### 2.2 配置文件命名规范

**默认配置路径**：
```
modules/[ModuleDir]/config.json
```
- `[ModuleDir]`: module 目录名称（如 Splitter-Cmn、G2p-Cmn）

**用户配置路径**：
```
configs/[moduleId].json
```
- `[moduleId]`: module ID（如 splitter-cmn、tagger-cmn、g2p-cmn）
- 使用小写字母和连字符，与 package.json 中的 moduleId 保持一致

**备份配置路径**：
```
config-backup/[moduleId].json.backup
```

---

## 3. 配置加载机制

### 3.1 配置加载优先级

```
用户配置（configs/） > 默认配置（modules/）
```

### 3.2 配置加载流程

```
1. 检查 configs/[moduleId].json 是否存在
   ├─ 存在 → 加载用户配置
   └─ 不存在 → 加载 modules/[ModuleDir]/config.json

2. 合并配置策略：
   - 用户配置完全覆盖默认配置
   - 缺失的字段使用默认配置的值

3. 验证配置：
   - 检查配置格式是否合法
   - 检查必需字段是否存在
   - 检查配置是否兼容当前 API Level
```

### 3.3 配置加载实现

```cpp
class ConfigLoader {
public:
    /// 加载 module 配置
    /// @param spec module 规范
    /// @return 加载的配置（合并后的配置）
    static Expected<JsonObject> loadConfig(const ModuleSpec *spec) {
        const auto &package = spec->parent();
        const auto &moduleId = spec->id();
        const auto &configPath = spec->manifestConfiguration();
        
        // 1. 尝试加载用户配置
        auto userConfigPath = package.path() / "configs" / (moduleId + ".json");
        if (std::filesystem::exists(userConfigPath)) {
            auto userConfig = loadJsonFile(userConfigPath);
            if (userConfig) {
                return userConfig.value();
            }
        }
        
        // 2. 加载默认配置
        auto defaultConfigPath = package.path() / configPath;
        auto defaultConfig = loadJsonFile(defaultConfigPath);
        if (!defaultConfig) {
            return Error(Error::FileSystemError, 
                        "Failed to load default config: " + defaultConfig.error().message());
        }
        
        return defaultConfig.value();
    }

private:
    static Expected<JsonObject> loadJsonFile(const std::filesystem::path &path) {
        // 实现文件读取和 JSON 解析
        // ...
    }
};
```

---

## 4. 配置保存机制

### 4.1 配置保存策略

1. **保存到用户配置目录**：configs/[moduleId].json
2. **自动创建目录**：如果 configs/ 目录不存在，自动创建
3. **原子操作**：先写入临时文件，然后重命名，确保数据完整性
4. **自动备份**：保存前自动备份到 config-backup/ 目录

### 4.2 配置保存流程

```
1. 准备保存：
   - 验证配置格式合法性
   - 检查配置是否兼容当前 API Level

2. 备份现有配置：
   - 如果 configs/[moduleId].json 存在，备份到 config-backup/

3. 原子保存：
   - 写入临时文件：configs/[moduleId].json.tmp
   - 原子重命名：configs/[moduleId].json.tmp → configs/[moduleId].json

4. 验证保存：
   - 读取保存的文件，验证内容正确性
```

### 4.3 配置保存实现

```cpp
class ConfigSaver {
public:
    /// 保存 module 配置
    /// @param spec module 规范
    /// @param config 要保存的配置
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> saveConfig(const ModuleSpec *spec, const std::string &config) {
        const auto &package = spec->parent();
        const auto &moduleId = spec->id();
        
        // 1. 验证配置格式
        auto configJson = parseJson(config);
        if (!configJson) {
            return configJson.error();
        }
        
        // 2. 创建用户配置目录
        auto configsDir = package.path() / "configs";
        std::filesystem::create_directories(configsDir);
        
        // 3. 备份现有配置
        auto userConfigPath = configsDir / (moduleId + ".json");
        if (std::filesystem::exists(userConfigPath)) {
            backupConfig(userConfigPath);
        }
        
        // 4. 原子保存
        auto tempPath = userConfigPath.string() + ".tmp";
        auto result = writeFile(tempPath, config);
        if (!result) {
            return result;
        }
        
        // 原子重命名
        std::filesystem::rename(tempPath, userConfigPath);
        
        return {};
    }

private:
    static void backupConfig(const std::filesystem::path &configPath) {
        auto backupDir = configPath.parent_path() / "../config-backup";
        std::filesystem::create_directories(backupDir);
        
        auto backupPath = backupDir / (configPath.filename().string() + ".backup");
        std::filesystem::copy_file(configPath, backupPath,
                                   std::filesystem::copy_options::overwrite_existing);
    }
};
```

---

## 5. 恢复默认配置机制

### 5.1 恢复策略

1. **删除用户配置**：删除 configs/[moduleId].json
2. **自动回退**：下次加载时自动使用 modules/ 中的默认配置
3. **保留备份**：config-backup/ 中的备份保留，便于恢复

### 5.2 恢复默认配置实现

```cpp
class ConfigResetter {
public:
    /// 恢复 module 的默认配置
    /// @param spec module 规范
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> resetToDefault(const ModuleSpec *spec) {
        const auto &package = spec->parent();
        const auto &moduleId = spec->id();
        
        // 1. 删除用户配置
        auto userConfigPath = package.path() / "configs" / (moduleId + ".json");
        if (std::filesystem::exists(userConfigPath)) {
            // 先备份
            backupConfig(userConfigPath);
            
            // 删除用户配置
            std::filesystem::remove(userConfigPath);
        }
        
        return {};
    }
    
    /// 检查是否使用默认配置
    /// @param spec module 规范
    /// @return true 如果使用默认配置，false 如果使用用户配置
    static bool isUsingDefaultConfig(const ModuleSpec *spec) {
        const auto &package = spec->parent();
        const auto &moduleId = spec->id();
        auto userConfigPath = package.path() / "configs" / (moduleId + ".json");
        return !std::filesystem::exists(userConfigPath);
    }
};
```

---

## 6. Task 配置接口集成

### 6.1 Task::getConfig() 实现

```cpp
std::string Task::getConfig() const {
    // 1. 获取当前配置
    auto config = ConfigLoader::loadConfig(spec());
    if (!config) {
        return "{}";
    }
    
    // 2. 序列化为 JSON 字符串
    return config.value().dump(2);
}
```

### 6.2 Task::setConfig() 实现

```cpp
Expected<void> Task::setConfig(const std::string &config) {
    // 1. 验证配置格式
    auto configJson = parseJson(config);
    if (!configJson) {
        return configJson.error();
    }
    
    // 2. 验证配置兼容性
    auto validation = validateConfig(configJson.value());
    if (!validation) {
        return validation;
    }
    
    // 3. 保存配置
    auto saveResult = ConfigSaver::saveConfig(spec(), config);
    if (!saveResult) {
        return saveResult;
    }
    
    // 4. 更新内部配置
    m_config = config;
    
    return {};
}
```

### 6.3 Task::resetToDefault() 新增接口

```cpp
/// Task 类新增方法
class Task {
public:
    // ... 现有方法 ...
    
    /// 恢复默认配置
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    virtual Expected<void> resetToDefault() {
        return ConfigResetter::resetToDefault(spec());
    }
    
    /// 检查是否使用默认配置
    /// @return true 如果使用默认配置，false 如果使用用户配置
    virtual bool isUsingDefaultConfig() const {
        return ConfigResetter::isUsingDefaultConfig(spec());
    }
};
```

---

## 7. 配置版本管理

### 7.1 配置版本格式

```json
{
  "$version": "1.0",
  "$schema": "module-config",
  "$modified": "2026-04-03T10:30:00Z",
  "$level": 1,
  "configuration": {
    // 实际配置内容
  }
}
```

### 7.2 配置版本升级

```cpp
class ConfigMigrator {
public:
    /// 升级配置到当前版本
    /// @param config 原始配置
    /// @param targetLevel 目标 API Level
    /// @return 升级后的配置
    static std::string migrateConfig(const std::string &config, int targetLevel) {
        auto configJson = parseJson(config);
        if (!configJson) {
            return config; // 解析失败，返回原配置
        }
        
        auto &obj = configJson.value();
        
        // 更新版本信息
        obj["$version"] = getCurrentVersion();
        obj["$level"] = targetLevel;
        obj["$modified"] = getCurrentTimestamp();
        
        // 执行版本特定的迁移逻辑
        // ...
        
        return obj.dump(2);
    }
};
```

---

## 8. 错误处理和恢复

### 8.1 配置加载错误处理

```cpp
Expected<JsonObject> loadConfig(const ModuleSpec *spec) {
    try {
        // 尝试加载用户配置
        auto userConfig = loadUserConfig(spec);
        if (userConfig) {
            return userConfig.value();
        }
        
        // 回退到默认配置
        auto defaultConfig = loadDefaultConfig(spec);
        if (defaultConfig) {
            Logger::warning("Failed to load user config, using default: %s", 
                           userConfig.error().message().c_str());
            return defaultConfig.value();
        }
        
        return defaultConfig.error();
    }
    catch (const std::exception &e) {
        Logger::error("Exception while loading config: %s", e.what());
        return Error(Error::RuntimeError, 
                    stdc::formatN("Failed to load config: %1", e.what()));
    }
}
```

### 8.2 配置保存错误恢复

```cpp
Expected<void> saveConfig(const ModuleSpec *spec, const std::string &config) {
    std::string backupPath;
    
    try {
        // 1. 备份现有配置
        backupPath = createBackup(spec);
        
        // 2. 保存新配置
        auto result = saveConfigAtomic(spec, config);
        if (!result) {
            // 保存失败，恢复备份
            restoreBackup(backupPath);
            return result;
        }
        
        // 3. 验证保存的配置
        auto verification = verifySavedConfig(spec);
        if (!verification) {
            // 验证失败，恢复备份
            restoreBackup(backupPath);
            return verification;
        }
        
        return {};
    }
    catch (const std::exception &e) {
        // 异常情况，尝试恢复备份
        if (!backupPath.empty()) {
            restoreBackup(backupPath);
        }
        return Error(Error::RuntimeError, 
                    stdc::formatN("Failed to save config: %1", e.what()));
    }
}
```

---

## 9. 配置管理 API

### 9.1 ConfigManager 类

```cpp
class LANGCORE_EXPORT ConfigManager {
public:
    /// 获取 module 配置
    /// @param spec module 规范
    /// @return 配置（JSON 字符串）
    static std::string getConfig(const ModuleSpec *spec);
    
    /// 设置 module 配置
    /// @param spec module 规范
    /// @param config 配置（JSON 字符串）
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> setConfig(const ModuleSpec *spec, const std::string &config);
    
    /// 恢复默认配置
    /// @param spec module 规范
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> resetToDefault(const ModuleSpec *spec);
    
    /// 检查是否使用默认配置
    /// @param spec module 规范
    /// @return true 如果使用默认配置，false 如果使用用户配置
    static bool isUsingDefaultConfig(const ModuleSpec *spec);
    
    /// 获取配置版本信息
    /// @param spec module 规范
    /// @return 配置版本信息
    static Expected<std::string> getConfigVersion(const ModuleSpec *spec);
    
    /// 导出配置到文件
    /// @param spec module 规范
    /// @param exportPath 导出路径
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> exportConfig(const ModuleSpec *spec, const std::filesystem::path &exportPath);
    
    /// 从文件导入配置
    /// @param spec module 规范
    /// @param importPath 导入路径
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> importConfig(const ModuleSpec *spec, const std::filesystem::path &importPath);
};
```

---

## 10. 向后兼容性保证

### 10.1 兼容性策略

1. **旧 package 无变化**：不包含 configs/ 目录的 package 继续正常工作
2. **自动创建目录**：首次保存配置时自动创建 configs/ 目录
3. **默认配置回退**：configs/ 不存在时自动使用 modules/ 中的配置
4. **配置格式兼容**：支持旧版本配置格式，自动升级

### 10.2 兼容性实现

```cpp
bool configsDirExists(const Package &package) {
    auto configsDir = package.path() / "configs";
    return std::filesystem::exists(configsDir);
}

bool shouldUseDefaultConfig(const Package &package, const std::string &moduleId) {
    if (!configsDirExists(package)) {
        return true; // configs/ 不存在，使用默认配置
    }
    
    auto userConfigPath = package.path() / "configs" / (moduleId + ".json");
    return !std::filesystem::exists(userConfigPath);
}
```

---

## 11. 安全性考虑

### 11.1 文件权限

1. **只读配置**：modules/ 目录标记为只读
2. **用户配置**：configs/ 目录权限可控
3. **备份保护**：config-backup/ 目录防止意外删除

### 11.2 数据完整性

1. **原子操作**：配置保存使用临时文件+重命名
2. **校验机制**：保存后验证文件内容
3. **备份恢复**：自动备份和恢复机制

---

## 12. 性能优化

### 12.1 配置缓存

```cpp
class ConfigCache {
private:
    static std::unordered_map<std::string, std::string> s_configCache;
    static std::shared_mutex s_cacheMutex;
    
public:
    static std::string getCachedConfig(const std::string &configPath) {
        std::shared_lock lock(s_cacheMutex);
        auto it = s_configCache.find(configPath);
        return it != s_configCache.end() ? it->second : "";
    }
    
    static void setCachedConfig(const std::string &configPath, const std::string &config) {
        std::unique_lock lock(s_cacheMutex);
        s_configCache[configPath] = config;
    }
    
    static void invalidateCache(const std::string &configPath) {
        std::unique_lock lock(s_cacheMutex);
        s_configCache.erase(configPath);
    }
};
```

### 12.2 延迟加载

```cpp
std::string Task::getConfig() const {
    // 检查缓存
    auto cacheKey = getConfigCacheKey();
    auto cached = ConfigCache::getCachedConfig(cacheKey);
    if (!cached.empty()) {
        return cached;
    }
    
    // 加载配置
    auto config = ConfigLoader::loadConfig(spec());
    if (config) {
        auto configStr = config.value().dump(2);
        
        // 更新缓存
        ConfigCache::setCachedConfig(cacheKey, configStr);
        
        return configStr;
    }
    
    return "{}";
}
```

---

## 13. 使用示例

### 13.1 基本使用

```cpp
// 获取配置
auto task = manager->task("g2p", "g2p-cmn");
std::string config = task->getConfig();
std::cout << config << std::endl;

// 设置配置
std::string newConfig = R"({
    "enabled": true,
    "customParam": "value"
})";
auto result = task->setConfig(newConfig);
if (!result) {
    std::cerr << "Failed to set config: " << result.error().message() << std::endl;
}

// 恢复默认配置
auto resetResult = task->resetToDefault();
if (!resetResult) {
    std::cerr << "Failed to reset config: " << resetResult.error().message() << std::endl;
}

// 检查是否使用默认配置
bool isDefault = task->isUsingDefaultConfig();
std::cout << "Using default config: " << (isDefault ? "yes" : "no") << std::endl;
```

### 13.2 配置管理器使用

```cpp
// 使用 ConfigManager
auto taskSpec = manager->task("g2p", "g2p-cmn")->spec();

// 获取配置
std::string config = ConfigManager::getConfig(taskSpec);

// 设置配置
auto result = ConfigManager::setConfig(taskSpec, newConfig);

// 恢复默认
ConfigManager::resetToDefault(taskSpec);

// 导出配置
ConfigManager::exportConfig(taskSpec, "exported_config.json");

// 导入配置
ConfigManager::importConfig(taskSpec, "imported_config.json");
```

---

## 14. 实现计划

### 14.1 实现步骤

1. **Phase 1: 核心基础设施**
   - 实现 ConfigLoader 类
   - 实现 ConfigSaver 类
   - 实现 ConfigResetter 类

2. **Phase 2: 配置管理器**
   - 实现 ConfigManager 类
   - 集成到 Task 类

3. **Phase 3: 缓存和优化**
   - 实现 ConfigCache 类
   - 实现延迟加载机制

4. **Phase 4: 错误处理**
   - 完善错误恢复机制
   - 添加配置验证

5. **Phase 5: 测试**
   - 单元测试
   - 集成测试
   - 性能测试

### 14.2 文件清单

**新增文件**：
- `core/include/LangCore/Support/ConfigLoader.h`
- `core/include/LangCore/Support/ConfigSaver.h`
- `core/include/LangCore/Support/ConfigResetter.h`
- `core/include/LangCore/Support/ConfigManager.h`
- `core/include/LangCore/Support/ConfigCache.h`
- `core/lib/Support/ConfigLoader.cpp`
- `core/lib/Support/ConfigSaver.cpp`
- `core/lib/Support/ConfigResetter.cpp`
- `core/lib/Support/ConfigManager.cpp`
- `core/lib/Support/ConfigCache.cpp`

**修改文件**：
- `core/include/LangCore/Task/Task.h`（添加新接口）
- `core/lib/Task/Task.cpp`（实现新接口）

---

## 15. 测试方案

### 15.1 单元测试

```cpp
TEST(ConfigPersistenceTest, LoadDefaultConfig) {
    // 测试加载默认配置
    auto spec = createTestSpec();
    auto config = ConfigLoader::loadConfig(spec);
    EXPECT_TRUE(config.ok());
    EXPECT_TRUE(config->contains("configuration"));
}

TEST(ConfigPersistenceTest, SaveUserConfig) {
    // 测试保存用户配置
    auto spec = createTestSpec();
    std::string testConfig = R"({"test": "value"})";
    auto result = ConfigSaver::saveConfig(spec, testConfig);
    EXPECT_TRUE(result.ok());
    
    // 验证配置已保存
    auto loaded = ConfigLoader::loadConfig(spec);
    EXPECT_TRUE(loaded.ok());
}

TEST(ConfigPersistenceTest, ResetToDefault) {
    // 测试恢复默认配置
    auto spec = createTestSpec();
    
    // 先设置用户配置
    ConfigSaver::saveConfig(spec, R"({"user": "config"})");
    
    // 恢复默认
    auto result = ConfigResetter::resetToDefault(spec);
    EXPECT_TRUE(result.ok());
    
    // 验证已恢复
    EXPECT_TRUE(ConfigResetter::isUsingDefaultConfig(spec));
}
```

### 15.2 集成测试

```cpp
TEST(ConfigPersistenceTest, TaskConfigIntegration) {
    // 测试 Task 配置接口集成
    auto task = createTestTask();
    
    // 获取配置
    auto config1 = task->getConfig();
    
    // 设置配置
    auto result = task->setConfig(R"({"custom": "value"})");
    EXPECT_TRUE(result.ok());
    
    // 验证配置
    auto config2 = task->getConfig();
    EXPECT_NE(config1, config2);
    
    // 恢复默认
    task->resetToDefault();
    auto config3 = task->getConfig();
    EXPECT_EQ(config1, config3);
}
```

---

## 16. 总结

本设计方案提供了完整的 config 持久化解决方案，具有以下特点：

### 16.1 核心优势

1. **向后兼容**：不破坏原有 package 结构
2. **按模块存储**：每个 module 独立配置管理
3. **默认分离**：默认配置和用户配置清晰分离
4. **灵活恢复**：支持恢复默认配置
5. **类型安全**：基于 Task 接口的一致性

### 16.2 技术特点

1. **原子操作**：配置保存采用原子操作，确保数据完整性
2. **自动备份**：保存前自动备份，防止数据丢失
3. **错误恢复**：完善的错误处理和恢复机制
4. **性能优化**：配置缓存和延迟加载提升性能

### 16.3 使用便捷

1. **API 一致**：与现有 Task 接口完全兼容
2. **自动管理**：自动处理配置加载、保存、恢复
3. **透明操作**：用户无需关心底层存储细节

---

**文档版本**：1.0
**创建日期**：2026-04-03
**作者**：Language Manager Team