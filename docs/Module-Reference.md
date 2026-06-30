# 模块参考手册

> 版本：3.0 | 更新日期：2026-04-27
>
> 架构总览请参阅 [Architecture-Overview.md](Architecture-Overview.md)，本文档仅列出公共头文件与插件的接口概要。

---

## §1 核心库 (`core/include/LangCore/`，共 31 个公共头文件)

### 1.1 Base/

| 头文件 | 说明 |
|--------|------|
| `AlignedAllocator.h` | 平台对齐内存分配器模板，供 Tensor 等需要 SIMD 对齐的容器使用。 |
| `LangCommon.h` | 公共数据结构：`TaggerRes`、`G2pInput`、`G2pRes`、`G2pErrorType` 枚举。所有插件共享的基础类型定义。 |
| `NamedObject.h` | 所有命名对象基类，提供 `NO<T>` 智能指针包装，用于对象池中的生命周期管理。 |
| `ObjectPool.h` | 命名对象容器，通过字符串 ID 进行 `add`/`remove`/`get` 操作。 |

### 1.2 Core/

| 头文件 | 说明 |
|--------|------|
| `Manager.h` | 单例管理器，核心入口。关键接口：`initialize()`、`task()`、`tasks()`、`convert()`。 |
| `ManagerLogger.h` | 预定义日志分类：`MgrLog`、`PluginLog`、`DependencyLog`、`ConfigLog`。 |
| `PackageManager.h` | 包发现、加载、依赖检查、模块类别注册。负责扫描 `G2pPackages/` 目录并构建模块索引。 |
| `Plugin.h` | 插件抽象接口与导出宏：`LANGCORE_EXPORT_PLUGIN`、`LANGCORE_DEFINE_TASK_PLUGIN`、`LANGCORE_DEFINE_DRIVER_PLUGIN`。 |
| `PluginFactory.h` | 插件动态库加载与查找。关键接口：`addPluginPath()`、`plugin<T>()`。 |

### 1.3 Module/

| 头文件 | 说明 |
|--------|------|
| `Module.h` | 定义 `ModuleLocator`、`ModuleSpec`、`ModuleCategory` 及相关宏。模块是包内可寻址的最小功能单元。 |
| `ModuleCategories.h` | 3 个内置类别声明：`Driver`、`G2p`、`Dict`。 |

#### Module/Dependency/

| 头文件 | 说明 |
|--------|------|
| `DependencyGraph.h` | 依赖图核心类型：`DependencyRequirement`、`ResolvedDependency`、`ModuleMetadata`、`DependencyGraph`、`PackageInitializationPlan`。 |
| `DependencyResolver.h` | 依赖解析器。关键接口：`resolveAllDependencies()`、`selectBestModules()`。 |
| `LevelCompatibilityChecker.h` | API Level 兼容性校验。规则：`M-1 <= P <= M`，Level 是唯一的 API 兼容性判据。 |
| `VersionUtils.h` | `VersionRange` 解析与 `VersionResolver`，用于 semver 范围过滤。 |

### 1.4 Package/

| 头文件 | 说明 |
|--------|------|
| `Package.h` | `Package` 句柄，包含 `id`、`version`、`moduleSpecs`、`path` 等字段。提供 `ScopedPackageRef` RAII 守卫。 |

### 1.5 Support/

| 头文件 | 说明 |
|--------|------|
| `ConfigAccessor.h` | 类型安全的配置访问接口，支持 `getString("key")` (必需) 和 `getString("key", "default")` (可选)。附带 `ValidationChain` 校验链。 |
| `DisplayText.h` | 多语言本地化文本容器。 |
| `Error.h` | `Error` 类，定义 11 种错误类型，每个错误包含 `message`、`suggestion`、`context`。 |
| `Expected.h` | `Expected<T>` 错误处理包装器，tagged union of T or Error。应用层逻辑禁止抛异常，统一使用此类型。 |
| `JSON.h` | `JsonValue`/`JsonArray`/`JsonObject`，对 nlohmann-json 的封装。 |
| `Logging.h` | `Logger` + `LogCategory` + 日志宏。格式占位符为 `%1`、`%2`（Qt 风格），`F` 后缀变体使用 printf 风格。 |
| `PhonemeDict.h` | 内存映射音素字典，提供 word → phoneme list 查询。 |
| `Tensor.h` | `ITensor` 接口与 `Tensor` CPU 实现，底层使用 `AlignedAllocator`。 |
| `ContextUtils.h` | `ContextKey`（context 名称 + 版本复合键），FQID 解析/格式化，context 名称/moduleId 校验。 |

### 1.6 Task/

| 头文件 | 说明 |
|--------|------|
| `Task.h` | 任务体系基础：`TaskInfoBase`、`TaskInput`、`TaskResult`、`TaskConfiguration`、`Task`（抽象）、`SessionTask`（抽象）。 |
| `TaskPlugin.h` | `TaskPlugin` (iid=`"org.openvpi.Task"`) 与 `DriverPlugin` (iid=`"org.openvpi.Driver"`)。 |
| `TaskFactory.h` | `SessionFactory` 接口，用于 AI 模型推理场景的会话创建。 |
| `G2pTask.h` | G2p 任务版本化输入输出：`G2pInputV1`、`G2pResultV1`。 |
| `DictTask.h` | 字典任务版本化输入输出：`DictInputV1`、`DictResV1`。 |
| `SessionTask.h` | 会话任务相关类型：`DriverInitArgs`、`SessionOpenArgs`、`SessionStartInput`、`SessionResult`、`ExecutionProvider` 枚举。 |
| `VersionedTaskImplBase.h` | 多版本任务实现的抽象基类，支持同一插件提供不同版本的任务接口。 |
| `VersionedTaskManager.h` | 版本化任务管理器，提供 `TASK_IMPLEMENT` 宏简化版本注册。 |

---

## §2 插件 (`plugins/`)

### 2.1 G2ps/

#### MandarinG2p

- **功能**：普通话 G2p，基于 cpp-pinyin
- **插件 key**：`g2p.template.MandarinG2pInference`
- **类名**：`LangPlugins::MandarinG2p`
- **目录**：`plugins/G2ps/MandarinG2p/`
- **依赖**：cpp-pinyin

#### CantoneseG2p

- **功能**：粤语 G2p
- **插件 key**：`g2p.template.CantoneseG2pInference`
- **类名**：`LangPlugins::CantoneseG2p`
- **目录**：`plugins/G2ps/CantoneseG2p/`
- **依赖**：cpp-pinyin

#### LstmG2p

- **功能**：LSTM 模型推理 G2p，基于 ONNX Runtime
- **插件 key**：`g2p.model.LstmG2pInference`
- **类名**：`LangPlugins::LstmG2p`
- **目录**：`plugins/G2ps/LstmG2p/`
- **依赖**：ONNX Runtime, OnnxDriver
- **备注**：支持 V1（逐词推理）和 V2（批量推理）两种任务版本

#### ChainG2p

- **功能**：责任链 G2p 框架，将多个处理步骤串联为流水线
- **插件 key**：`g2p.chain.ChainG2pInference`
- **类名**：`LangPlugins::ChainG2p`
- **目录**：`plugins/G2ps/ChainG2p/`
- **依赖**：按配置依赖其他 G2p/Dict 插件
- **步骤类型**：`TagAndValidate`、`Dict`、`Model`、`Fallback`、`Format`

### 2.2 Dicts/

#### DsDict

- **功能**：字典查询插件
- **插件 key**：`dict.dsdict`
- **类名**：`LangPlugins::DsDict`
- **目录**：`plugins/Dicts/DsDict/`
- **依赖**：无外部依赖

### 2.3 Drivers/

#### OnnxDriver

- **功能**：ONNX Runtime 推理驱动
- **插件 key**：`onnx`
- **目录**：`plugins/Drivers/OnnxDriver/`
- **依赖**：ONNX Runtime
- **支持的 ExecutionProvider**：CPU、CUDA、DirectML、CoreML

### 2.4 Utils/

| 插件 | 功能 | 目录 |
|------|------|------|
| Common | 公共工具，包含 `PluginValidationUtils` | `plugins/Utils/Common/` |
| InferUtil | 推理工具，包含 `Parser` | `plugins/Utils/InferUtil/` |
| OnnxUtil | ONNX 相关工具函数 | `plugins/Utils/OnnxUtil/` |

---

## §3 资源包 (`res/G2pPackages/`)

| 包名 | packageId | 包含模块 | 备注 |
|------|-----------|----------|------|
| Phonetic-Suite-Cmn | `cmn-official` | `g2p-cmn-official` | 普通话 |
| Phonetic-Suite-Eng | `eng-official` | `g2p-lstm-eng-official`, `g2p-eng-official` | 英语，ChainG2p 配置，依赖 LstmG2p |
| Phonetic-Suite-Jpn | `jpn-official` | `g2p-jpn-official` | 日语 |
| Phonetic-Suite-Yue | `yue-official` | `g2p-yue-official` | 粤语 |
| Phonetic-Suite-Num | `num-official` | `g2p-num-official` | 数字 |
| Phonetic-Suite-Punc | `punc-official` | `g2p-punc-official` | 标点 |
| Phonetic-Suite-Unknown | `unknown-official` | `g2p-unknown-official` | 未知语种兜底 |

---

## §4 测试 (`tests/`)

### catch2/ (L1/L2 单元与组件测试)

所有 Catch2 测试合并为单一可执行目标 `LangMgrTests`，不依赖插件和运行时环境。测试框架为 Catch2 v2.13.10 单头文件（位于 `tests/common/catch.hpp`）。

| 文件 | 覆盖范围 |
|------|----------|
| `tst_base_types.cpp` | `LangCommon` 基础类型 |
| `tst_error.cpp` | `Error` 类型与错误码 |
| `tst_expected.cpp` | `Expected<T>` 值/错误语义 |
| `tst_error_expected.cpp` | `Error`、`Expected<T>` 综合用例 |
| `tst_json_config.cpp` | `JSON`、`ConfigAccessor` |
| `tst_version_dep.cpp` | `VersionUtils`、依赖解析 |
| `tst_dependency_graph.cpp` | `DependencyGraph`（Kahn 拓扑排序） |
| `tst_dependency_resolver.cpp` | `DependencyResolver` |
| `tst_fqid.cpp` | FQID 解析/格式化, context 名称校验, moduleId 校验 |
| `tst_context_validation.cpp` | Context/版本字段合法性校验 |
| `tst_context_convert.cpp` | G2pInput/G2pRes context 字段, convert API context 校验 |
| `tst_context_isolation.cpp` | Context 隔离, 跨 context 依赖失败, 默认 context 回退 |
| `tst_context_dedup.cpp` | 模块去重 (isSameMainModule), selectBestModules |
| `tst_context_version.cpp` | ContextKey, 带版本 FQID, 版本化 context 隔离/回退/去重 |

### tst_langCore/ (L4 集成测试)

需要插件和资源包就绪后运行。包含本地 Splitter/Tagger 实现及简化 JSON 配置（位于 `tests/tst_langCore/configs/`）。不使用 Catch2，为独立可执行目标。
