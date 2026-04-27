# Language Manager 编码规范与标准

**版本**：2.0  
**日期**：2026-04-27  
**适用范围**：所有向 Language Manager 贡献代码的开发者

---

## §1 代码格式

项目使用 `.clang-format` 统一格式化，基于 LLVM 风格定制：

- C++17 标准
- 缩进：4 空格
- 行宽上限：120 列
- `NamespaceIndentation: All`（命名空间内容缩进）
- `BraceWrapping.AfterNamespace: true`（命名空间左花括号换行）

提交前请确保代码通过 clang-format 格式化。

---

## §2 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 插件类 | `[Name]Plugin` | `MandarinG2pPlugin` |
| 任务类 | `[Name]Task` | `MandarinG2pTask` |
| 命名空间 | `LangPlugins::[Name]` | `LangPlugins::MandarinG2p` |
| 插件 key | `category.plugin-name` | `g2p.template.MandarinG2pInference` |
| 模块类别宏 | `LANGCORE_DECLARE_MODULE_CATEGORY(Name, Key)` | `LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")` |
| 日志分类 | `LangCore::LogCategory Log("name")` | `LangCore::LogCategory Log("onnxDriver")` |

### Level 与 Version

- **Level** 是整数（1, 2, 3...），标识 API 结构版本。
- **Version** 是语义化版本号，格式为 `MAJOR.MINOR.PATCH`。
- Version 首位应与 Level 保持一致。例如 Level=1 对应 Version=1.x.x。

---

## §3 错误处理规范

### 基本原则

1. 应用层逻辑使用 `Expected<T>` 传播错误，**禁止抛出异常**。
2. `try-catch` **仅**用于第三方库边界，将外部异常转为 `Error`。
3. 每个 `catch` 块必须记录日志或返回 `Error`，**禁止静默吞掉**。
4. 禁止在业务逻辑中使用 `try-catch` 做流程控制。

### 允许使用 try-catch 的第三方库边界

| 边界 | 来源 | 捕获类型 |
|------|------|----------|
| JSON 解析 | nlohmann/json | `std::exception` |
| ONNX 推理 | ONNX Runtime | `Ort::Exception` |
| 正则验证 | `std::regex` | `std::regex_error` |
| 版本号解析 | `std::stoi` | `...` |
| 拼音转换 | cpp-pinyin | `std::exception` |
| 类型转换 | `std::any_cast` | `std::bad_any_cast` |

新增第三方库集成时，在调用入口处统一捕获并转为 `Expected<T>`。

### Error 类型

```
Success, ConfigError, FileSystemError, DependencyError, RuntimeError,
NotImplementedError, InitializationError, ValidationError, NullPointerError,
IndexError, TimeoutError
```

`Error` 可附带上下文信息：

```cpp
Error(type, msg)
    .withContext(file, line, function)   // 定位信息
    .withExtra(extra);                   // 额外上下文
```

构造时也可携带修复建议：

```cpp
Error(type, msg, suggestion);
```

### 领域层错误

G2p 转换使用独立的错误类型 `G2pErrorType`，与框架层 `Error` 互不合并：

```
NoError, InvalidLyric, ModelInferenceFailed,
PhonemeGenerationFailed, DriverUnavailable, UnknownError
```

---

## §4 日志规范

### 基本用法

```cpp
#include <LangCore/Support/Logging.h>

// 声明日志分类
LangCore::LogCategory Log("pluginName");

// Qt-style 占位符（%1, %2, ...）
Log.langCoreInfo("Task initialized: %1", taskId);
Log.langCoreWarning("Config missing key: %1", key);

// printf-style 变体
Log.langCoreInfoF("Loaded %d entries", count);
```

### 日志级别

Trace, Debug, Success, Information, Warning, Critical, Fatal

### 内置分类

| 分类 | 用途 |
|------|------|
| `MgrLog` | Manager 层操作（初始化、包加载） |
| `PluginLog` | 插件扫描、DLL 加载 |
| `DependencyLog` | 依赖解析、Level/Version 校验 |
| `ConfigLog` | 配置读取、验证 |

这些分类定义在 `ManagerLogger.h` 中。

---

## §5 配置访问规范

### ConfigAccessor

```cpp
auto cfg = LangCore::config(spec());
```

### 字段访问

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `getString("key")` | `Expected<std::string>` | 必需字段，缺失即报错 |
| `getString("key", "default")` | `std::string` | 可选字段，提供默认值 |
| `getInt("key")` / `getInt("key", 0)` | 同上模式 | 整数 |
| `getDouble("key")` / `getDouble("key", 0.5)` | 同上模式 | 浮点数 |
| `getBool("key")` / `getBool("key", true)` | 同上模式 | 布尔值 |
| `getPath("key")` | `Expected<path>` | 解析为基于模块目录的绝对路径 |
| `getStringArray("key")` | `Expected<vector<string>>` | 字符串数组 |

可用 `cfg.has("key")` 检查字段是否存在。

### ValidationChain

支持链式验证，返回第一个失败的错误：

```cpp
ValidationChain()
    .validateIntRange(batchSize, 1, 1000, "batchSize")
    .validateStringAllowed(mode, {"standard", "fast"}, "mode")
    .validateArrayNotEmpty(regexes, "regexes")
    .execute();
```

### 配置来源

模块的 `config.json` 由 `ModuleSpec::manifestConfiguration()` 提供。插件在 `initialize()` 中调用 `initializeConfig()` 完成配置加载。

---

## §6 插件导出规范

### TaskPlugin 导出

```cpp
LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, "category.key", ApiLevel)
```

一行完成插件类定义和 C 导出函数生成。

### DriverPlugin 导出

```cpp
LANGCORE_DEFINE_DRIVER_PLUGIN(PluginClass, FactoryClass, "key", ApiLevel)
```

两个宏内部均调用 `LANGCORE_EXPORT_PLUGIN`。

### 单版本 Task

```cpp
TASK_IMPLEMENT(TaskClass, ImplClass)
```

生成构造函数和全部委托方法。

### 多版本 Task

手动编写构造函数，按 `spec->apiLevel()` 选择实现：

```cpp
MyTask::MyTask(const ModuleSpec *spec) : Task(spec), _manager(spec) {
    switch (spec->apiLevel()) {
        case 2: _manager.setImpl(std::make_unique<V2::TaskImpl>(spec)); break;
        default: _manager.setImpl(std::make_unique<V1::TaskImpl>(spec)); break;
    }
}
TASK_IMPLEMENT_METHODS(MyTask)
```

`TASK_IMPLEMENT_METHODS` 仅生成委托方法（`initialize`、`start`、`getConfig`），构造函数由开发者手动编写。

---

## §7 Package 格式规范

### 基本结构

扩展名 `.lmpk`，本质是 UTF-8 编码的 ZIP 文件。

```
my-package.lmpk
├── package.json
├── modules/
│   └── G2p-Cmn/
│       └── config.json
└── assets/
```

### package.json

**必选字段**：

- `packageId`：包标识符，禁止包含 `/\[]:;'"` 字符

**可选字段**：

- `version`、`vendor`、`copyright`、`description`、`url`、`modules`

`modules` 按类别组织（`g2p` / `driver` / `dict`），每个模块声明 `moduleId`、`class`（对应插件 key）、`configuration`（指向 config.json 路径）、`dependencies`（可选）。

### 模块 config.json

```json
{
  "$version": "1.0.0",
  "level": 1,
  ...
}
```

`$version` 和 `level` 由 `PackageManager` 在依赖解析阶段读取。其余字段由具体插件通过 `ConfigAccessor` 自行解析。

---

## §8 Level 与 Version 规范

### Level

整数，决定 Core API 结构兼容性。函数签名、结构体布局变更时递增。

### Version

语义化版本号（`MAJOR.MINOR.PATCH`），描述同一 Level 下的内部实现差异。

### 兼容条件

```
Manager Level = M, Plugin Level = P
兼容条件: M-1 <= P <= M
```

工具插件（Driver 等）不受此规则限制。

### 依赖解析双重校验

当模块 A 依赖模块 B 时：

1. 先按 `level` 过滤候选模块（精确匹配）
2. 再按 `version` 范围过滤（支持 `>=`、`~`、`*`、连字符范围等语法）
3. 从满足条件的候选中选取最高版本

Level 超出兼容范围的插件直接拒绝加载并输出清晰的错误日志，不做降级适配。

---

## §9 设计原则

| 原则 | 含义 |
|------|------|
| 简洁可靠 | 遇错直接返回，不设计重试或回滚 |
| 接口稳定 | Level 锚定 API 结构兼容性，公共头文件即契约 |
| 长期免维护 | 插件加载后常驻内存，无复杂生命周期管理 |
| 可接受的不兼容 | Level 超出范围直接拒绝，不降级 |
| 异常边界隔离 | `Expected<T>` 传播应用层错误，`try-catch` 仅限第三方库边界 |

---

## §10 Context 命名规范

### ContextKey

`ContextKey` 是 `(context, version)` 复合键，定义于 `ContextUtils.h`：

| 状态 | context | version | 示例 |
|------|---------|---------|------|
| 默认 context | `""` | 空 | `ContextKey()` |
| 无版本 context | 非空 | 空 | `ContextKey("SingerA")` |
| 带版本 context | 非空 | 非空 | `ContextKey("SingerA", {2,0,0})` |

**默认 context 不可带版本**——空字符串 + 非空版本号是非法组合。

### Context 名称规则

| 规则 | 说明 |
|------|------|
| 允许字符 | `[A-Za-z0-9_.-]` |
| 禁止字符 | `:` `/` `\` `[` `]` `;` `'` `"` 空格 |
| 最大长度 | 128 字符 |
| 空字符串 | 合法，表示默认 context |

验证方法：`ContextUtils::validateContextName()`。

### 分隔符约定

| 分隔符 | 用途 | 示例 |
|--------|------|------|
| `@` | 分隔 context 与 version | `SingerA@2.0.0` |
| `:` | 分隔 context 与 moduleId | `SingerA:g2p-cmn` |

### FQID（完全限定标识符）

完整格式：`context@version:moduleId`

| 输入 | 解析结果 |
|------|----------|
| `SingerA@2.0.0:g2p-cmn` | context=`SingerA`, version=`2.0.0`, moduleId=`g2p-cmn` |
| `SingerA:g2p-cmn` | context=`SingerA`, version=空, moduleId=`g2p-cmn` |
| `g2p-cmn` | context=`""`, version=空, moduleId=`g2p-cmn` |
| `:g2p-cmn` | context=`""`, version=空, moduleId=`g2p-cmn` |

解析方法：`ContextUtils::parseFqid()`。格式化方法：`ContextUtils::formatFqid()`。

### moduleId 约束

- **禁止包含 `:`**——冒号是 FQID 中 context 与 moduleId 的分隔符
- 验证方法：`ContextUtils::validateModuleId()`

---

**文档版本**: 2.0  
**最后更新**: 2026-04-27
