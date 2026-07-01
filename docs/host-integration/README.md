# LangCore 宿主集成指南

本目录为 `language-manager`（LangCore）G2P 框架的宿主集成文档，面向 `ds-editor-lite`、`synthrt` 等宿主工程的接入与测试设计。所有文档基于已核实的代码事实（Code Facts）编写，引用源码时给出 `file:///` 链接以便直接跳转。

> 2026-07-02 修订（第二轮）：依据 synthrt `docs/g2p-design/` 权威设计，对齐 D10-D13 新决策：
> - **D10**：`G2pInput`/`G2pRes` 字段统一为 `g2pContext`/`g2pContextVersion`，新增 `g2pSource`；`ContextKey` 保持 `context`/`version`（框架内部 API）；`g2pSource` 由 `Manager::convert()` 依据 `G2pInput.g2pContext` 填充（详见 [REFACTOR-PLAN.md](REFACTOR-PLAN.md) T1/T2）。
> - **D11**：宿主侧回退策略统一为 `Never`-only，框架 `convert()` 行为不变（本就无策略）。
> - **D12**：`G2pErrorType` 为本仓库独家定义的跨项目唯一错误契约；`G2pOutcome.errorType` 已是 `LangCore::G2pErrorType`（现状）；**但调用方 `G2pResult` 当前无 `errorType` 字段**（仅有 `fellBackToOfficial`），D11 删除后需 D12 配套新增 `errorType`。
> - **D13**：重构范围 = 文档重构 + 接口稳定化，详见 [REFACTOR-PLAN.md](REFACTOR-PLAN.md)。
>
> 2026-07-02 修订（第一轮）：依据全链路代码核对，修正 FQID 格式、`addPluginPath` IID、`initialize()` 幂等行为、`addPackagePath` 后置行为、tagger/splitter 分类等多处事实错误。

## 框架定位

LangCore 是一套 **G2P（Grapheme-to-Phoneme）基础设施框架**，职责包括：

- **插件加载与管理**：通过 IID（`org.openvpi.Driver` / `org.openvpi.Task`）加载 Driver / G2p 等插件，通过分类（category）机制暴露 `driver` / `g2p` / `dict` 等对象池。
- **G2P 包扫描与依赖解析**：`PackageManager` 扫描 G2P 包目录，解析模块元数据与依赖关系。
- **上下文隔离（Context Isolation）**：以 `(context, version, g2pId)` 三维路由实现「官方默认上下文 + 声库私有上下文」隔离，保证声库自定义 G2P 与官方 G2P 互不污染。
- **G2P 转换入口**：对外暴露 `Manager::convert()` 作为批量 G2P 转换的唯一入口。

> 注：宿主侧的 `G2pConvertRunner`（D11 后简化为无策略封装）并不在 LangCore 内部，而位于 `ds-editor-lite`；LangCore 仅提供无策略的原子 `convert()`。

## 宿主集成概览

| 宿主 | 集成方式 |
| --- | --- |
| `ds-editor-lite` | 完整集成：加载 Drivers/G2ps 插件、注册官方 + 声库私有 G2P 包、初始化 ONNX 驱动、通过 `G2pConvertRunner`（D11 统一 `Never` 策略）调用 `convert()` |
| `synthrt` | 同类集成：添加插件路径与包路径，初始化后调用 `convert()` |

典型集成步骤（详见 [03-host-integration-contract.md](03-host-integration-contract.md)）：

1. `addPluginPath(iid, path)` —— 按 IID 添加插件搜索路径（`org.openvpi.Driver` / `org.openvpi.Task`）
2. `addPackagePath(context, [version], path)` —— 先注册官方默认上下文（`context=""`），再注册声库私有上下文
3. ONNX 驱动初始化，以裸名 `g2pOnnxDriver` 注册到 `driver` 分类（不参与上下文隔离）
4. `Manager::initialize()` —— 完成初始化（success-gated 幂等）
5. `Manager::convert()` —— 执行 G2P 转换

## 文档索引

| 编号 | 文档 | 内容 |
| --- | --- | --- |
| 01 | [01-framework-capabilities.md](01-framework-capabilities.md) | 框架能力审计：Manager API、PackageManager、ContextState 状态机、G2pRes、插件系统、幂等守卫 |
| 02 | [02-context-isolation-mechanism.md](02-context-isolation-mechanism.md) | 上下文隔离机制：三维路由模型、ContextKey/FQID 格式、默认/私有上下文、S5 漏洞修复、跨上下文依赖、命名校验 |
| 03 | [03-host-integration-contract.md](03-host-integration-contract.md) | 宿主集成契约：必须做 / 禁止做、加载约束 L-1~L-4、初始化顺序、ONNX 注册模式、错误处理 |
| 04 | [04-test-design.md](04-test-design.md) | 测试设计：双层测试策略、三大测试领域（S5 多上下文 / 路由两级决策 / 启动时序）、实现计划 |

## 关键设计原则

1. **上下文隔离优先**：声库私有 G2P 与官方 G2P 必须互不可见（除显式依赖回退），防止命名冲突与污染。
2. **无策略原子转换**：框架 `convert()` 本身无回退策略；宿主侧 D11 后统一为 `Never`（不回退），失败仅 copy fallback + 精准 `G2pErrorType` 上报（D12）。
3. **启动期加载**：自定义 G2P 仅在启动时加载，运行时新增声库需重启（约束 L-1、L-3）。
4. **success-gated 幂等**：`initialize()` 成功后再次调用返回 `Error::AlreadyInitialized`；失败可重试（约束 L-2）。
5. **单点失败不阻塞**：单个私有上下文解析失败不阻塞整体初始化，通过 `collectError` 模式收集错误；默认上下文失败则整体中止。
6. **基础设施全局化**：ONNX 驱动作为全局基础设施，以裸名注册，不绑定到任何私有上下文。
7. **全链路字段统一（D10）**：`G2pInput`/`G2pRes` 字段统一为 `g2pContext`/`g2pContextVersion`，新增 `g2pSource`；`ContextKey` 保持 `context`/`version`（内部 API）；调用方 `G2pRoute`/`G2pRequest`/`G2pOutcome` 同步重命名；简化 Layer 2 字段名映射层（`G2pOutcome`→`G2pResult`），Layer 1 类型转换层（std↔Qt）保留。
8. **错误类型独家定义（D12）**：`G2pErrorType` 由本仓库独家定义并跨项目透传；`G2pOutcome.errorType` 已是 `LangCore::G2pErrorType`（现状）；**但调用方 `G2pResult` 当前无 `errorType`**（仅有 `fellBackToOfficial`），D11 删除后需 D12 配套新增。

## 决策历史摘要

### 设计决策（D1–D7）

| 决策 | 内容 | 说明 |
| --- | --- | --- |
| D1 | 采用 `(context, version, g2pId)` 三维路由 | 区分官方与声库私有 G2P，支持同 g2pId 多版本并存 |
| D2 | 默认上下文 `context=""` | 空字符串表示官方全局上下文，所有声库可见作为兜底 |
| D3 | FQID 格式：默认上下文为**裸 moduleId**（无冒号）；私有上下文为 `context:moduleId` 或 `context@version:moduleId` | 通过 FQID 显式定位模块；`formatFqid` 的 `isDefault()` 分支直接返回 moduleId |
| D4 | ModelStep 两级查找（S5 修复） | 私有上下文未命中时回退默认上下文（裸 moduleId），解决模型模块仅在默认上下文注册的问题 |
| D5 | ONNX 驱动裸名注册 | `g2pOnnxDriver` 注册到 `driver` 分类，不参与上下文隔离，作为全局基础设施 |
| D6 | initialize() success-gated 幂等 | 成功后重调返回 `Error::AlreadyInitialized`；失败可重试（`initialized` 仅成功末尾置 true） |
| D7 | 错误收集而非中断 | `collectError` 模式：单私有上下文失败收集错误，继续解析其余；默认上下文失败则整体中止 |

### 跨项目决策（D10–D13，权威来源：synthrt `docs/g2p-design/05-design-decisions-history.md`）

| 决策 | 内容 | 对本仓库的影响 |
| --- | --- | --- |
| D10 | `G2pInput`/`G2pRes` 字段统一为 `g2pContext`/`g2pContextVersion`，新增 `g2pSource`；`ContextKey` 保持 `context`/`version`（内部 API） | 详见 [REFACTOR-PLAN.md](REFACTOR-PLAN.md) T1/T2 |
| D11 | 宿主侧回退策略统一 `Never`-only | 框架 `convert()` 行为不变（本就无策略）；宿主侧移除 `ToOfficial` |
| D12 | `G2pErrorType` 为本仓库独家定义的跨项目唯一错误契约 | 本仓库无代码变更（已独家定义）；调用方 `G2pResult` 需新增 `errorType`（D11 删 `fellBackToOfficial` 的配套） |
| D13 | 重构范围 = 文档重构 + 接口稳定化 | 详见 [REFACTOR-PLAN.md](REFACTOR-PLAN.md) |

### 加载约束（L-1～L-4）

| 约束 | 内容 | 实际保证机制 |
| --- | --- | --- |
| L-1 | 自定义 G2P 仅在启动时加载 | `addPackagePath` 不检查 `initialized`，但新路径因 `initialize()` 不可重跑而永不被处理 |
| L-2 | `initialize()` success-gated 幂等 | 成功后返回 `Error::AlreadyInitialized`；失败可重试 |
| L-3 | 运行时新增声库需重启 | L-1 推论 |
| L-4 | 官方上下文先于私有上下文初始化 | 由 `initialize()` Phase 1（默认）/ Phase 2（私有）顺序保证，与 `addPackagePath` 调用顺序无关 |

## 相关源码入口

- Manager 单例：`core/include/LangCore/Core/Manager.h`
- Manager 实现：`core/lib/Core/Manager.cpp`
- PackageManager / ContextState：`core/include/LangCore/Core/PackageManager.h`
- ContextKey / FQID / 命名校验：`core/include/LangCore/Support/ContextUtils.h`
- 公共类型（G2pRes / G2pInput 等）：`core/include/LangCore/Base/LangCommon.h`
- ModelStep S5 修复：`plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
- Catch2 测试：`tests/catch2/`
- 端到端测试：`tests/tst_langCore/main.cpp`
