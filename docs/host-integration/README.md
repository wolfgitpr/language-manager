# LangCore 宿主集成指南

本目录为 `language-manager`（LangCore）G2P 框架的宿主集成文档，面向 `ds-editor-lite`、`synthrt` 等宿主工程的接入与测试设计。所有文档基于已核实的代码事实（Code Facts）编写，引用源码时尽量给出 `file:///` 链接以便直接跳转。

## 框架定位

LangCore 是一套 **G2P（Grapheme-to-Phoneme）基础设施框架**，职责包括：

- **插件加载与管理**：通过分类（category）机制加载 Driver / G2p / Tagger / Splitter 等插件。
- **G2P 包扫描与依赖解析**：`PackageManager` 扫描 G2P 包目录，解析模块元数据与依赖关系。
- **上下文隔离（Context Isolation）**：以 `(context, version, g2pId)` 三维路由实现「官方默认上下文 + 声库私有上下文」隔离，保证声库自定义 G2P 与官方 G2P 互不污染。
- **G2P 转换入口**：对外暴露 `Manager::convert()` 作为批量 G2P 转换的唯一入口。

> 注：宿主侧的 `G2pConvertRunner`（带回退策略的封装）并不在 LangCore 内部，而位于 `ds-editor-lite`；LangCore 仅提供无策略的原子 `convert()`。

## 宿主集成概览

| 宿主 | 集成方式 |
| --- | --- |
| `ds-editor-lite` | 完整集成：加载 Drivers/G2ps 插件、注册官方 + 声库私有 G2P 包、初始化 ONNX 驱动、通过 `G2pConvertRunner`（ToOfficial / Never 策略）调用 `convert()` |
| `synthrt` | 同类集成：添加插件路径与包路径，初始化后调用 `convert()` |

典型集成步骤（详见 [03-host-integration-contract.md](03-host-integration-contract.md)）：

1. `addPluginPath` —— 添加 Drivers / G2ps / Taggers / Splitters 搜索路径
2. `addPackagePath` —— 先注册官方默认上下文，再注册声库私有上下文
3. 初始化 ONNX 驱动，以裸名 `g2pOnnxDriver` 注册到 `driver` 分类（不参与上下文隔离）
4. `Manager::initialize()` —— 完成初始化（幂等）
5. `Manager::convert()` —— 执行 G2P 转换

## 文档索引

| 编号 | 文档 | 内容 |
| --- | --- | --- |
| 01 | [01-framework-capabilities.md](01-framework-capabilities.md) | 框架能力审计：Manager API、PackageManager、ContextState 状态机、G2pRes、插件系统、幂等守卫 |
| 02 | [02-context-isolation-mechanism.md](02-context-isolation-mechanism.md) | 上下文隔离机制：三维路由模型、默认/私有上下文、S5 漏洞修复、跨上下文依赖、命名校验 |
| 03 | [03-host-integration-contract.md](03-host-integration-contract.md) | 宿主集成契约：必须做 / 禁止做、加载约束 L-1~L-4、初始化顺序、ONNX 注册模式、错误处理 |
| 04 | [04-test-design.md](04-test-design.md) | 测试设计：双层测试策略、三大测试领域（S5 多上下文 / 路由两级决策 / 启动时序）、实现计划 |

## 关键设计原则

1. **上下文隔离优先**：声库私有 G2P 与官方 G2P 必须互不可见（除显式回退），防止命名冲突与污染。
2. **官方兜底**：当私有上下文转换失败时，允许通过 `ToOfficial` 策略回退到官方默认上下文。
3. **启动期加载**：自定义 G2P 仅在启动时加载，运行时新增声库需重启（约束 L-1、L-3）。
4. **幂等初始化**：`initialize()` 不可重复执行，`addPackagePath()` 必须在 `initialize()` 之前调用（约束 L-2）。
5. **单点失败不阻塞**：单个 G2P 包解析失败不应阻塞整体初始化，通过 `collectError` 模式收集错误。
6. **基础设施全局化**：ONNX 驱动作为全局基础设施，以裸名注册，不绑定到任何私有上下文。

## 决策历史摘要

### 设计决策（D1–D7）

| 决策 | 内容 | 说明 |
| --- | --- | --- |
| D1 | 采用 `(context, version, g2pId)` 三维路由 | 区分官方与声库私有 G2P，支持同 g2pId 多版本并存 |
| D2 | 默认上下文 `context=""` | 空字符串表示官方全局上下文，所有声库可见作为兜底 |
| D3 | FQID 格式 `context:g2pId` | 通过 FQID（Fully Qualified ID）显式定位模块，默认上下文前缀为空 |
| D4 | ModelStep 两级查找（S5 修复） | 私有上下文未命中时回退默认上下文，解决模型模块仅在默认上下文注册的问题 |
| D5 | ONNX 驱动裸名注册 | `g2pOnnxDriver` 注册到 `driver` 分类，不参与上下文隔离，作为全局基础设施 |
| D6 | initialize() 幂等守卫 | 已初始化后再次调用为 no-op，防止重复初始化破坏状态 |
| D7 | 错误收集而非中断 | `collectError` 模式：单包失败收集错误，继续解析其余包 |

### 加载约束（L-1～L-4）

| 约束 | 内容 |
| --- | --- |
| L-1 | 自定义 G2P 仅在启动时加载 |
| L-2 | `initialize()` 幂等，不可重复调用 |
| L-3 | 运行时新增声库需重启 |
| L-4 | 官方上下文必须在私有上下文之前注册 |

## 相关源码入口

- Manager 单例：`core/include/LangCore/Core/Manager.h`
- Manager 实现：`core/lib/Core/Manager.cpp`
- PackageManager / ContextState：`core/include/LangCore/Core/PackageManager.h`
- 公共类型（G2pRes / G2pInput 等）：`core/include/LangCore/Base/LangCommon.h`
- ModelStep S5 修复：`plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
- Catch2 测试：`tests/catch2/`
- 端到端测试：`tests/tst_langCore/main.cpp`
