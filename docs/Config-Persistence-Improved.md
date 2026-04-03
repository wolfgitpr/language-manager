# Config 持久化改进方案

## 1. 改进空间分析

### 1.1 当前方案的问题

1. **路径计算重复**：每次加载/保存配置都要重新计算路径
2. **配置加载时机不明确**：Task 何时加载配置？何时合并配置？
3. **自动更新缺失**：没有明确的配置自动更新机制
4. **接口不一致**：resetToDefault 接口不够灵活

### 1.2 改进方向

1. **ModuleSpec 内部路径缓存**：避免重复计算配置路径
2. **Task 生命周期集成**：在 Task::initialize 时自动加载配置
3. **自动保存机制**：Task::setConfig 自动保存到用户配置目录
4. **智能恢复接口**：支持选择性恢复默认配置

---

## 2. ModuleSpec 内部路径维护

### 2.1 配置路径缓存设计

```cpp
// ModuleSpec 内部实现
class ModuleSpec::Impl {
public:
    // ... 现有成员 ...

    // 配置路径缓存
    std::filesystem::path defaultConfigPath;  // 默认配置路径
    std::filesystem::path userConfigPath;     // 用户配置路径
    std::filesystem::path backupConfigPath;    // 备份配置路径
    std::filesystem::path configsDir;         // 用户配置目录
    bool pathsInitialized = false;            // 路径是否已初始化

    void initializeConfigPaths() {
        if (pathsInitialized) return;

        auto packagePath = parent->path();
        auto &configPathStr = manifestConfiguration;
        
        // 解析配置路径
        auto configRelativePath = configPathStr.toString();
        
        // 设置默认配置路径
        defaultConfigPath = packagePath / configRelativePath;
        
        // 设置用户配置目录和路径
        configsDir = packagePath / "configs";
        userConfigPath = configsDir / (id + ".json");
        
        // 设置备份配置路径
        auto backupDir = packagePath / "config-backup";
        backupConfigPath = backupDir / (id + ".json.backup");
        
        pathsInitialized = true;
    }
};
```

### 2.2 路径访问接口

```cpp
// ModuleSpec 新增接口
class ModuleSpec {
public:
    // ... 现有接口 ...

    /// 获取默认配置路径
    const std::filesystem::path &defaultConfigPath() const;
    
    /// 获取用户配置路径
    const std::filesystem::path &userConfigPath() const;
    
    /// 获取备份配置路径
    const std::filesystem::path &backupConfigPath() const;
    
    /// 获取用户配置目录
    const std::filesystem::path &configsDir() const;
    
    /// 检查用户配置是否存在
    bool hasUserConfig() const;
};
```

---

## 3. Task 配置生命周期集成

### 3.1 Task::initialize() 自动加载配置

```cpp
Expected<void> Task::initialize() {
    // 1. 初始化配置路径（ModuleSpec 内部）
    spec()->impl().initializeConfigPaths();
    
    // 2. 加载配置（优先用户配置，回退到默认配置）
    auto config = loadTaskConfig();
    if (!config) {
        return config.error();
    }
    
    // 3. 设置内部配置
    m_config = config.value();
    
    // 4. 调用子类初始化
    return onInitialize();
}

Expected<std::string> Task::loadTaskConfig() {
    // 1. 检查用户配置是否存在
    if (spec()->hasUserConfig()) {
        return loadConfigFile(spec()->userConfigPath());
    }
    
    // 2. 加载默认配置
    return loadConfigFile(spec()->defaultConfigPath());
}

Expected<std::string> Task::loadConfigFile(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        return Error(Error::FileSystemError, 
                    "Config file not found: " + path.string());
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return Error(Error::FileSystemError,
                    "Failed to open config file: " + path.string());
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    return content;
}
```

### 3.2 Task::setConfig() 自动保存

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
    
    // 3. 保存到用户配置目录
    auto saveResult = saveUserConfig(config);
    if (!saveResult) {
        return saveResult;
    }
    
    // 4. 更新内部配置
    m_config = config;
    
    // 5. 通知配置变更（可选）
    onConfigChanged(config);
    
    return {};
}

Expected<void> Task::saveUserConfig(const std::string &config) {
    auto &configsDir = spec()->configsDir();
    
    // 1. 创建用户配置目录
    std::filesystem::create_directories(configsDir);
    
    // 2. 备份现有配置
    auto &userConfigPath = spec()->userConfigPath();
    if (std::filesystem::exists(userConfigPath)) {
        backupConfig(userConfigPath);
    }
    
    // 3. 原子保存
    auto tempPath = userConfigPath.string() + ".tmp";
    {
        std::ofstream file(tempPath);
        if (!file.is_open()) {
            return Error(Error::FileSystemError,
                        "Failed to create temp config file");
        }
        file << config;
    }
    
    // 原子重命名
    std::filesystem::rename(tempPath, userConfigPath);
    
    return {};
}
```

---

## 4. 自动更新配置时机

### 4.1 配置自动更新场景

1. **首次初始化**：Task::initialize() 时自动加载配置
2. **用户修改配置**：Task::setConfig() 时自动保存配置
3. **插件升级**：插件版本更新时检查配置兼容性
4. **配置迁移**：API Level 变更时自动迁移配置

### 4.2 配置兼容性检查

```cpp
class Task {
protected:
    /// 配置兼容性检查
    /// @param config 要检查的配置
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    virtual Expected<void> validateConfig(const JsonObject &config) {
        // 1. 检查配置版本
        if (config.contains("$version")) {
            auto version = config["$version"].toString();
            auto currentVersion = getConfigVersion();
            if (version != currentVersion) {
                return migrateConfig(config, version, currentVersion);
            }
        }
        
        // 2. 检查 API Level 兼容性
        if (config.contains("$level")) {
            auto configLevel = config["$level"].toInt();
            if (configLevel != spec()->apiLevel()) {
                return Error(Error::ConfigError,
                            "Config level mismatch: config level " + std::to_string(configLevel) +
                            " != API level " + std::to_string(spec()->apiLevel()));
            }
        }
        
        // 3. 检查必需字段
        return validateRequiredFields(config);
    }
    
    /// 配置迁移
    /// @param config 要迁移的配置
    /// @param fromVersion 来源版本
    /// @param toVersion 目标版本
    /// @return 迁移后的配置
    virtual Expected<JsonObject> migrateConfig(
        const JsonObject &config,
        const std::string &fromVersion,
        const std::string &toVersion
    ) {
        // 默认实现：不进行迁移
        return config;
    }
    
    /// 获取配置版本
    /// @return 配置版本字符串
    virtual std::string getConfigVersion() const {
        return "1.0";
    }
};
```

---

## 5. 优化 Task 恢复默认设置接口

### 5.1 恢复选项设计

```cpp
enum class ConfigResetOption {
    /// 完全恢复默认配置（删除用户配置）
    FullReset,
    
    /// 恢复默认配置但保留某些字段
    SelectiveReset,
    
    /// 创建配置快照（保存当前配置到备份）
    CreateSnapshot,
    
    /// 从快照恢复
    RestoreFromSnapshot
};

class ConfigResetOptions {
public:
    ConfigResetOptions(ConfigResetOption option = ConfigResetOption::FullReset)
        : _option(option) {}
    
    /// 设置选择性恢复的保留字段
    ConfigResetOptions &setPreservedFields(const std::vector<std::string> &fields) {
        _preservedFields = fields;
        return *this;
    }
    
    /// 设置快照名称
    ConfigResetOptions &setSnapshotName(const std::string &name) {
        _snapshotName = name;
        return *this;
    }
    
    ConfigResetOption option() const { return _option; }
    const std::vector<std::string> &preservedFields() const { return _preservedFields; }
    const std::string &snapshotName() const { return _snapshotName; }

private:
    ConfigResetOption _option;
    std::vector<std::string> _preservedFields;
    std::string _snapshotName;
};
```

### 5.2 优化的 resetToDefault 接口

```cpp
class Task {
public:
    // ... 现有接口 ...
    
    /// 恢复默认配置（默认选项）
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    Expected<void> resetToDefault() {
        return resetToDefault(ConfigResetOptions());
    }
    
    /// 恢复默认配置（带选项）
    /// @param options 恢复选项
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    Expected<void> resetToDefault(const ConfigResetOptions &options) {
        switch (options.option()) {
            case ConfigResetOption::FullReset:
                return fullResetToDefault();
            
            case ConfigResetOption::SelectiveReset:
                return selectiveResetToDefault(options.preservedFields());
            
            case ConfigResetOption::CreateSnapshot:
                return createConfigSnapshot(options.snapshotName());
            
            case ConfigResetOption::RestoreFromSnapshot:
                return restoreFromSnapshot(options.snapshotName());
        }
    }
    
    /// 检查是否使用默认配置
    /// @return true 如果使用默认配置，false 如果使用用户配置
    bool isUsingDefaultConfig() const {
        return !spec()->hasUserConfig();
    }
    
    /// 获取配置修改时间
    /// @return 配置修改时间，如果不存在返回 std::nullopt
    std::optional<std::filesystem::file_time_type> getConfigModifiedTime() const {
        auto &userConfigPath = spec()->userConfigPath();
        if (!std::filesystem::exists(userConfigPath)) {
            return std::nullopt;
        }
        return std::filesystem::last_write_time(userConfigPath);
    }

protected:
    /// 完全恢复默认配置
    Expected<void> fullResetToDefault() {
        auto &userConfigPath = spec()->userConfigPath();
        
        if (!std::filesystem::exists(userConfigPath)) {
            // 用户配置不存在，已经在使用默认配置
            return {};
        }
        
        // 备份当前配置
        backupConfig(userConfigPath);
        
        // 删除用户配置
        std::filesystem::remove(userConfigPath);
        
        // 重新加载默认配置
        auto loadResult = loadTaskConfig();
        if (!loadResult) {
            return loadResult.error();
        }
        
        m_config = loadResult.value();
        
        return {};
    }
    
    /// 选择性恢复默认配置
    /// @param preservedFields 要保留的用户配置字段
    Expected<void> selectiveResetToDefault(const std::vector<std::string> &preservedFields) {
        // 1. 加载默认配置
        auto defaultConfigResult = loadConfigFile(spec()->defaultConfigPath());
        if (!defaultConfigResult) {
            return defaultConfigResult.error();
        }
        
        auto defaultConfig = parseJson(defaultConfigResult.value()).value();
        
        // 2. 如果存在用户配置，合并保留字段
        if (spec()->hasUserConfig()) {
            auto userConfigResult = loadConfigFile(spec()->userConfigPath());
            if (userConfigResult) {
                auto userConfig = parseJson(userConfigResult.value()).value();
                
                // 合并保留字段
                for (const auto &field : preservedFields) {
                    if (userConfig.contains(field)) {
                        defaultConfig[field] = userConfig[field];
                    }
                }
            }
        }
        
        // 3. 保存合并后的配置
        auto configStr = defaultConfig.dump(2);
        auto saveResult = saveUserConfig(configStr);
        if (!saveResult) {
            return saveResult;
        }
        
        m_config = configStr;
        
        return {};
    }
    
    /// 创建配置快照
    /// @param snapshotName 快照名称
    Expected<void> createConfigSnapshot(const std::string &snapshotName) {
        if (snapshotName.empty()) {
            return Error(Error::InvalidArgument, "Snapshot name cannot be empty");
        }
        
        // 生成快照文件名
        auto snapshotPath = spec()->configsDir() / 
                          ("snapshots/" + spec()->id() + "-" + snapshotName + ".json");
        
        // 创建快照目录
        std::filesystem::create_directories(snapshotPath.parent_path());
        
        // 保存当前配置
        std::ofstream file(snapshotPath);
        if (!file.is_open()) {
            return Error(Error::FileSystemError,
                        "Failed to create snapshot file");
        }
        file << m_config;
        
        return {};
    }
    
    /// 从快照恢复
    /// @param snapshotName 快照名称
    Expected<void> restoreFromSnapshot(const std::string &snapshotName) {
        if (snapshotName.empty()) {
            return Error(Error::InvalidArgument, "Snapshot name cannot be empty");
        }
        
        // 查找快照文件
        auto snapshotPath = spec()->configsDir() / 
                          ("snapshots/" + spec()->id() + "-" + snapshotName + ".json");
        
        if (!std::filesystem::exists(snapshotPath)) {
            return Error(Error::FileSystemError,
                        "Snapshot not found: " + snapshotName);
        }
        
        // 读取快照配置
        auto configResult = loadConfigFile(snapshotPath);
        if (!configResult) {
            return configResult.error();
        }
        
        // 验证快照配置
        auto configJson = parseJson(configResult.value());
        if (!configJson) {
            return configJson.error();
        }
        
        auto validation = validateConfig(configJson.value());
        if (!validation) {
            return validation;
        }
        
        // 保存快照配置
        auto saveResult = saveUserConfig(configResult.value());
        if (!saveResult) {
            return saveResult;
        }
        
        m_config = configResult.value();
        
        return {};
    }
    
    /// 列出所有快照
    /// @return 快照名称列表
    std::vector<std::string> listConfigSnapshots() const {
        std::vector<std::string> snapshots;
        
        auto snapshotsDir = spec()->configsDir() / "snapshots";
        if (!std::filesystem::exists(snapshotsDir)) {
            return snapshots;
        }
        
        auto prefix = spec()->id() + "-";
        for (const auto &entry : std::filesystem::directory_iterator(snapshotsDir)) {
            if (entry.is_regular_file()) {
                auto filename = entry.path().filename().string();
                if (filename.rfind(prefix) == 0 && 
                    filename.length() > prefix.length() + 1 &&
                    filename.substr(filename.length() - 5) == ".json") {
                    auto snapshotName = filename.substr(
                        prefix.length(), 
                        filename.length() - prefix.length() - 5
                    );
                    snapshots.push_back(snapshotName);
                }
            }
        }
        
        return snapshots;
    }
    
    /// 删除快照
    /// @param snapshotName 快照名称
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    Expected<void> deleteConfigSnapshot(const std::string &snapshotName) {
        auto snapshotPath = spec()->configsDir() / 
                          ("snapshots/" + spec()->id() + "-" + snapshotName + ".json");
        
        if (!std::filesystem::exists(snapshotPath)) {
            return Error(Error::FileSystemError,
                        "Snapshot not found: " + snapshotName);
        }
        
        std::filesystem::remove(snapshotPath);
        return {};
    }
    
    /// 配置变更回调（子类可重写）
    /// @param newConfig 新配置
    virtual void onConfigChanged(const std::string &newConfig) {
        // 默认实现：什么都不做
    }
    
    /// 验证必需字段（子类可重写）
    /// @param config 配置对象
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    virtual Expected<void> validateRequiredFields(const JsonObject &config) {
        return {}; // 默认不验证
    }
};
```

---

## 6. 配置管理器实现

### 6.1 ConfigManager 类

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
    
    /// 恢复默认配置（带选项）
    /// @param spec module 规范
    /// @param options 恢复选项
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> resetToDefault(const ModuleSpec *spec, const ConfigResetOptions &options);
    
    /// 检查是否使用默认配置
    /// @param spec module 规范
    /// @return true 如果使用默认配置，false 如果使用用户配置
    static bool isUsingDefaultConfig(const ModuleSpec *spec);
    
    /// 获取配置修改时间
    /// @param spec module 规范
    /// @return 配置修改时间，如果不存在返回 std::nullopt
    static std::optional<std::filesystem::file_time_type> getConfigModifiedTime(const ModuleSpec *spec);
    
    /// 创建配置快照
    /// @param spec module 规范
    /// @param snapshotName 快照名称
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> createConfigSnapshot(const ModuleSpec *spec, const std::string &snapshotName);
    
    /// 从快照恢复
    /// @param spec module 规范
    /// @param snapshotName 快照名称
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> restoreFromSnapshot(const ModuleSpec *spec, const std::string &snapshotName);
    
    /// 列出所有快照
    /// @param spec module 规范
    /// @return 快照名称列表
    static std::vector<std::string> listConfigSnapshots(const ModuleSpec *spec);
    
    /// 删除快照
    /// @param spec module 规范
    /// @param snapshotName 快照名称
    /// @return 成功返回 Expected<void>::success()，失败返回错误
    static Expected<void> deleteConfigSnapshot(const ModuleSpec *spec, const std::string &snapshotName);
    
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
    
    /// 批量配置操作
    /// @param specs module 规范列表
    /// @param operation 配置操作
    /// @return 批量操作结果
    static std::vector<std::pair<std::string, Expected<void>>> 
        batchConfigOperation(
            const std::vector<const ModuleSpec *> &specs,
            std::function<Expected<void>(const ModuleSpec *)> operation
        );
};
```

---

## 7. 配置版本管理

### 7.1 配置版本格式

```json
{
  "$version": "1.0",
  "$schema": "module-config",
  "$created": "2026-04-03T10:30:00Z",
  "$modified": "2026-04-03T11:00:00Z",
  "$level": 1,
  "$module": "g2p-cmn",
  "configuration": {
    // 实际配置内容
  }
}
```

### 7.2 配置版本管理实现

```cpp
class ConfigVersionManager {
public:
    /// 添加版本信息到配置
    /// @param config 配置对象
    /// @param moduleId module ID
    /// @param apiLevel API Level
    /// @return 带版本信息的配置
    static JsonObject addVersionInfo(
        const JsonObject &config,
        const std::string &moduleId,
        int apiLevel
    ) {
        JsonObject versionedConfig = config;
        
        versionedConfig["$version"] = getCurrentVersion();
        versionedConfig["$schema"] = "module-config";
        versionedConfig["$created"] = getCurrentTimestamp();
        versionedConfig["$modified"] = getCurrentTimestamp();
        versionedConfig["$level"] = apiLevel;
        versionedConfig["$module"] = moduleId;
        
        return versionedConfig;
    }
    
    /// 更新修改时间
    /// @param config 配置对象
    /// @return 更新后的配置
    static JsonObject updateModifiedTime(const JsonObject &config) {
        JsonObject updated = config;
        updated["$modified"] = getCurrentTimestamp();
        return updated;
    }
    
    /// 获取配置版本
    /// @param config 配置对象
    /// @return 配置版本
    static std::string getConfigVersion(const JsonObject &config) {
        if (config.contains("$version")) {
            return config["$version"].toString();
        }
        return "unknown";
    }
    
    /// 获取配置 API Level
    /// @param config 配置对象
    /// @return API Level
    static int getConfigLevel(const JsonObject &config) {
        if (config.contains("$level")) {
            return config["$level"].toInt();
        }
        return 0;
    }
    
private:
    static std::string getCurrentVersion() {
        return "1.0"; // 配置格式版本
    }
    
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        char buffer[30];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
        return std::string(buffer);
    }
};
```

---

## 8. 实现文件清单

### 8.1 新增文件

**配置持久化核心类**：
- `core/include/LangCore/Support/ConfigLoader.h`
- `core/lib/Support/ConfigLoader.cpp`
- `core/include/LangCore/Support/ConfigSaver.h`
- `core/lib/Support/ConfigSaver.cpp`
- `core/include/LangCore/Support/ConfigManager.h`
- `core/lib/Support/ConfigManager.cpp`

**配置版本管理**：
- `core/include/LangCore/Support/ConfigVersionManager.h`
- `core/lib/Support/ConfigVersionManager.cpp`

**配置重置选项**：
- `core/include/LangCore/Support/ConfigResetOptions.h`

### 8.2 修改文件

**ModuleSpec 扩展**：
- `core/include/LangCore/Module/Module.h`（添加配置路径接口）
- `core/lib/Module/Module.cpp`（实现配置路径缓存）

**Task 类扩展**：
- `core/include/LangCore/Task/Task.h`（添加新接口）
- `core/lib/Task/Task.cpp`（实现配置生命周期集成）

---

## 9. 测试方案

### 9.1 单元测试

```cpp
TEST(ConfigPersistenceTest, ModuleSpecPathCache) {
    // 测试 ModuleSpec 路径缓存
    auto spec = createTestSpec();
    
    // 第一次调用，初始化路径
    auto path1 = spec->userConfigPath();
    auto path2 = spec->userConfigPath();
    EXPECT_EQ(path1, path2); // 路径应该相同
    
    // 验证路径格式
    EXPECT_FALSE(path1.empty());
    EXPECT_TRUE(path1.string().ends_with(".json"));
}

TEST(ConfigPersistenceTest, TaskAutoLoadConfig) {
    // 测试 Task 自动加载配置
    auto task = createTestTask();
    auto result = task->initialize();
    EXPECT_TRUE(result.ok());
    
    // 验证配置已加载
    auto config = task->getConfig();
    EXPECT_FALSE(config.empty());
}

TEST(ConfigPersistenceTest, TaskAutoSaveConfig) {
    // 测试 Task 自动保存配置
    auto task = createTestTask();
    task->initialize();
    
    // 设置配置
    auto result = task->setConfig(R"({"test": "value"})");
    EXPECT_TRUE(result.ok());
    
    // 验证配置已保存到用户配置目录
    EXPECT_TRUE(task->hasUserConfig());
    EXPECT_FALSE(task->isUsingDefaultConfig());
}

TEST(ConfigPersistenceTest, SelectiveReset) {
    // 测试选择性恢复
    auto task = createTestTask();
    task->initialize();
    
    // 设置用户配置
    task->setConfig(R"({"custom": "value", "preserved": "keep"})");
    
    // 选择性恢复，保留 "preserved" 字段
    ConfigResetOptions options(ConfigResetOption::SelectiveReset);
    options.setPreservedFields({"preserved"});
    
    auto result = task->resetToDefault(options);
    EXPECT_TRUE(result.ok());
    
    // 验证保留的字段仍然存在
    auto config = parseJson(task->getConfig()).value();
    EXPECT_TRUE(config.contains("preserved"));
}

TEST(ConfigPersistenceTest, ConfigSnapshot) {
    // 测试配置快照
    auto task = createTestTask();
    task->initialize();
    
    // 设置配置
    task->setConfig(R"({"version": "1.0"})");
    
    // 创建快照
    auto result = task->createConfigSnapshot("test-snapshot");
    EXPECT_TRUE(result.ok());
    
    // 修改配置
    task->setConfig(R"({"version": "2.0"})");
    
    // 从快照恢复
    auto restoreResult = task->restoreFromSnapshot("test-snapshot");
    EXPECT_TRUE(restoreResult.ok());
    
    // 验证已恢复
    auto config = parseJson(task->getConfig()).value();
    EXPECT_EQ(config["version"].toString(), "1.0");
}
```

---

## 10. 使用示例

### 10.1 基本使用

```cpp
// 自动加载配置（在 Task::initialize 时）
auto task = manager->task("g2p", "g2p-cmn");
task->initialize(); // 自动加载配置

// 自动保存配置（在 Task::setConfig 时）
auto result = task->setConfig(R"({"enabled": true})");
if (result) {
    // 配置已自动保存到用户配置目录
}

// 检查配置状态
bool isDefault = task->isUsingDefaultConfig();
auto modifiedTime = task->getConfigModifiedTime();
```

### 10.2 恢复默认配置

```cpp
// 完全恢复默认配置
auto result = task->resetToDefault();

// 选择性恢复，保留某些字段
ConfigResetOptions options(ConfigResetOption::SelectiveReset);
options.setPreservedFields({"userSettings", "customPreferences"});
auto result = task->resetToDefault(options);

// 创建配置快照
task->createConfigSnapshot("before-upgrade");

// 从快照恢复
task->restoreFromSnapshot("before-upgrade");

// 列出所有快照
auto snapshots = task->listConfigSnapshots();

// 删除快照
task->deleteConfigSnapshot("old-snapshot");
```

### 10.3 配置管理器使用

```cpp
auto taskSpec = manager->task("g2p", "g2p-cmn")->spec();

// 获取配置
std::string config = ConfigManager::getConfig(taskSpec);

// 设置配置
ConfigManager::setConfig(taskSpec, R"({"test": "value"})");

// 恢复默认
ConfigManager::resetToDefault(taskSpec);

// 选择性恢复
ConfigResetOptions options(ConfigResetOption::SelectiveReset);
options.setPreservedFields({"custom"});
ConfigManager::resetToDefault(taskSpec, options);

// 快照管理
ConfigManager::createConfigSnapshot(taskSpec, "backup");
ConfigManager::restoreFromSnapshot(taskSpec, "backup");
auto snapshots = ConfigManager::listConfigSnapshots(taskSpec);
ConfigManager::deleteConfigSnapshot(taskSpec, "old");
```

---

## 11. 总结

### 11.1 改进成果

1. **ModuleSpec 路径缓存**：避免重复计算配置路径
2. **Task 生命周期集成**：自动加载和保存配置
3. **智能恢复接口**：支持多种恢复选项
4. **配置快照功能**：支持配置版本管理
5. **自动更新机制**：在合适的时机自动更新配置

### 11.2 核心优势

1. **性能优化**：路径缓存和延迟加载
2. **自动化**：配置管理自动化，减少手动操作
3. **灵活性**：多种恢复选项和快照功能
4. **向后兼容**：不破坏原有 package 结构
5. **类型安全**：完全基于 Task 接口的一致性

### 11.3 使用便捷

1. **零配置**：Task 自动管理配置生命周期
2. **智能合并**：自动合并默认配置和用户配置
3. **版本管理**：支持配置快照和版本回滚
4. **安全可靠**：原子操作和自动备份机制

---

**文档版本**：2.0
**创建日期**：2026-04-03
**作者**：Language Manager Team