# 01 — 现状审计

> **日期**: 2026-07-01
> **核实方式**: 逐文件阅读实际代码，区分"已实现"与"文档声称但未实现"
> **整合自**: 原 `lang-framework-plan/01-current-state-audit.md` + `refactoring/00-overview.md`

---

## 1. 核心框架实际能力（v3.1，已实现）

### 1.1 Context 隔离路由（核心机制，稳定可用）

**实际实现**：`ContextKey{context, version}` 双维度，代码与 v3.1 文档一致（v4.0 简化方案已撤销，从未落地）。

| 能力 | 状态 | 代码位置 |
|------|------|---------|
| `ContextKey` 结构体（context + version） | ✅ | [ContextUtils.h:17-54](file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h) |
| `G2pInput.contextVersion` / `G2pRes.contextVersion` | ✅ | [LangCommon.h:23-77](file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h) |
| `addPackagePath(context, path)` 两参重载 | ✅ | [PackageManager.h:44](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) |
| `addPackagePath(context, version, path)` 三参重载 | ✅ | [PackageManager.h:50](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) |
| R-8 校验：默认 context 不允许带 version | ✅ | [PackageManager.cpp:552-608](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) |
| `task(category, ctx, id)` 三参 + `task(category, ctx, ver, id)` 四参 | ✅ | [Manager.h:29-31](file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h) |
| 版本化查找回退链（exact → unversioned） | ✅ | [Manager.cpp:361-500](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) |
| 两阶段初始化（默认 context 先行） | ✅ | [Manager.cpp:61-252](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) |
| 依赖解析回退到默认 context（Dep-2） | ✅ | [DependencyResolver.cpp:80-92](file:///D:/projects/language-manager/core/lib/Module/Dependency/DependencyResolver.cpp) |
| `convert()` 不跨 context 回退（C-6） | ✅ | [Manager.cpp:361-500](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) |
| `ContextUtils::validateContextName` | ✅ | [ContextUtils.h:112-134](file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h) |

### 1.2 插件体系（稳定可用）

| 插件 | Key | 版本实现 | 备注 |
|------|-----|---------|------|
| ChainG2p | `g2p.chain.ChainG2pInference` | V1 | Pipeline/Step 体系 |
| LstmG2p | `g2p.model.LstmG2pInference` | V1（单词）+ V2（批量） | V1 仅处理首词，**务必用 level=2** |
| MandarinG2p | `g2p.template.MandarinG2pInference` | V1 | cpp-pinyin |
| CantoneseG2p | `g2p.template.CantoneseG2pInference` | V1 | cpp-pinyin jyutping |
| DsDict | — | V1 | 字典查表 |
| OnnxDriver | `onnx` | — | ONNX Runtime 后端 |

### 1.3 测试覆盖（catch2 单元测试 + tst_langCore 集成测试）

- `tests/catch2/` 14 个测试文件覆盖：context 隔离、版本化、依赖解析、FQID、错误传播
- `tests/tst_langCore/` 集成测试：**只注册默认 context**，不测试多 context 端到端（盲区，见 §3.5）

---

## 2. 关键漏洞（P0，必须修复）

### 2.1 VULN-1 · ModelStep 不感知 context（S5 场景失败）

**严重级别**: P0 | **影响**: S5 场景（声库私有 ChainG2p + 声库私有 LstmG2p）静默降级

**问题**：[ModelStep.cpp:44-61](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp) 中：

```cpp
auto g2pCate = spec->Mgr()->category("g2p");
auto g2pObj = g2pCate->getFirstObject(m_onnxG2pId);  // ← 裸 id，不带 context 前缀
```

`createModuleTask` 在 [PackageManager.cpp:862](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) 注册 task 时用 FQID（`ContextUtils::formatFqid(ContextKey(context, version), moduleId)`），对非默认 context 变成 `"SingerA:g2p-lstm-eng-official"`。

**后果**：声库私有 ChainG2p 的 ModelStep 用裸 `m_onnxG2pId` 查找，找不到声库私有 LstmG2p（它注册为 `SingerA:g2p-lstm-eng-official`），`m_enabled = false` 静默降级，单词原样返回。

**对应 synthrt 建议**：[O-2 ModelStep FQID 两级查找](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md)

**修复**：见 [02-interface-design.md §3](02-interface-design.md)。

---

## 3. 架构债清单

### 3.1 AD-F1 · 三层 public 继承暴露 deprecated 方法（P2）

**问题**：`Manager` public 继承 `PackageManager` public 继承 `PluginFactory`。`PackageManager` 的三个 deprecated 方法通过继承对宿主可见（[PackageManager.h:34-39, 62-64](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h)）：

- `checkDependencies()` — 跨 context 扁平化，与隔离设计冲突
- `getPackageInitializationOrder()` — 同上
- `loadPackagesInOrder()` — 同上

三者已 `@deprecated` 标注，`Manager::initialize()` 内部不调用，但宿主仍可误调用导致跨 context 扁平化行为。

**影响**：宿主易误用，破坏 context 隔离。

**修复**：分两 Level 推进（v3 对齐修订，详见 [02-interface-design.md §2](02-interface-design.md) 与 [05 §ARCH-02](05-design-principles-check.md)）：
- 当前 Level（v3.x）：保留 public 可见性，追加 `[[deprecated]]` 编译期警告
- 下一 Level（v4.x）：将三个方法私有化并递增 Level

已确认宿主 ds-editor-lite 未调用这三个方法。

### 3.2 AD-F2 · `contextStates` 私有，失败 context 不可观测（P2）

**问题**：[PackageManager::Impl](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) 的 `contextStates`（现状为 `enum class ContextState { Pending, Ready, Failed }` 三态）是私有成员。宿主无法主动发现哪些 context 初始化失败，只能通过 `task()` 调用返回 `RuntimeError` 反推。

**影响**：宿主无法在启动后一次性报告"哪些声库 G2P 不可用"，用户体验差。

**对应 synthrt 建议**：[O-1 failedContexts() API](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md)（synthrt 标注为"必需"）

**修复**：新增公共方法 `contextState() / failedContexts()`（[02-interface-design.md §4](02-interface-design.md)）。

> **v3 对齐修订**：`ContextState` 枚举从三态扩展为四态 `Pending` / `Ready` / `Failed` / `NotRegistered`。`contextState()` 未命中（未注册）返回 `NotRegistered`（而非 `Pending`），以区分"未注册"与"已注册但未初始化"。与 SingerInfo.resolutionState 三态（Resolved/Pending/Missing）分层独立。详见 [02 §4.2](02-interface-design.md)。

### 3.3 AD-F3 · `PackageManager::open()` 传递依赖未实现（P2）

**问题**：[PackageManager.cpp:145-151, 251](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) 中 `llvm::SmallVector<PackageData *> dependencies` 始终为空，`closeDependencies` lambda 和 `pkg.linked` 操作的都是空向量。`open()` 单独加载包不会自动加载其依赖。

**影响**：宿主若依赖 `open()` 单独加载会得到不完整结果。但 `Manager::initialize()` 通过 `loadPackagesInOrder` 拓扑加载不受影响。

**修复**：不实现传递依赖（避免过度设计）；明确文档说明 `open()` 不解析依赖，宿主应使用 `addPackagePath + initialize` 全流程。

### 3.4 AD-F4 · `convert()` 失败语义用 mode="copy" 表达（P3）

**问题**：`convert()` 失败时返回的 `G2pRes` 的 `mode` 仍是 `"copy"`，与"copy fallback 成功"无法区分，必须检查 `errorType != NoError`。

**影响**：宿主需记住此约定，易出错。

**修复**：新增 `G2pRes::isOk() / isFailed()` 便利方法（`errorType == NoError` 即成功）；文档明确语义。不改 `mode` 字段语义（避免破坏性变更）。

### 3.5 AD-F5 · 多 context 端到端测试缺失（P2）

**问题**：`tests/tst_langCore/main.cpp:94` 只注册默认 context（`addPackagePath("", packagesRootDir)`），不测试多 context 端到端 convert。S5 漏洞未被测试发现。

**影响**：context 隔离的端到端正确性无回归防护。

**修复**：新增多 context 集成测试（[04-implementation-tasks.md](04-implementation-tasks.md) 任务 1.5）。

### 3.6 AD-F6 · `G2pRes.mode` 语义易误用（P2）

**问题**：核实 [Manager.cpp:361-500](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `convert()` 实现，`mode` 字段有三种取值（`"convert"`/`"copy"`/`"skip"`），但与 `errorType` 的组合关系不直观：

| `mode` | `errorType` | 含义 |
|--------|-------------|------|
| `"convert"` | `NoError` | 插件成功转换 |
| `"copy"` | `NoError` | 插件合法的原词保留（标点/数字，如 FallbackStep） |
| `"copy"` | 非 `NoError` | `convert()` 内部失败兜底 |
| `"skip"` | `NoError` | 空 lyric 跳过 |

宿主侧若仅检查 `mode == "copy"` 判定失败，会误判插件合法的原词保留为失败。

**修复**：新增 `G2pRes::isOk() / isFailed()` 便利方法（[02-interface-design §5](02-interface-design.md)），固化判定语义。

### 3.7 AD-F7 · `Manager::task()` 对 Pending context 错误信息不精确（P3）

**问题**：[Manager.cpp:282-319](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `task()` 查找时，若 context 处于 `Pending` 状态（已注册路径但尚未 initialize），返回 `T-6: could not find context`，与"从未注册"（C-5）错误信息相同，难以区分。

**影响**：宿主调试时无法区分"context 未注册"与"context 已注册但未初始化"。

**修复**：`task()` 内部检查 `contextState`，对 Pending 状态返回更精确的错误信息。优先级低，属于 DX 改进。

### 3.8 AD-F8 · `Manager::initialize()` 缺少幂等性防护（P3）

**问题**：[Manager.cpp](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `initialize()` 未做"已初始化"防护，二次调用会重跑全流程（非增量），可能导致已加载 task 状态不一致。

**对应 synthrt 建议**：[O-5 initialize() 幂等性防护](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md)（synthrt 标注 L-2 硬约束）

**修复**：见 [02-interface-design.md §6](02-interface-design.md)。

---

## 4. 技术债清单（P1/P2，✅ 全部已完成）

> 原始清单来自已删除的 `refactoring/02-technical-debt.md`。全部 11 项已在提交 `f17ac89..c810e82` 中修复完成，详见 git 历史。下表保留问题记录以供溯源。

### 4.1 P1 技术债（✅ 已完成）

| 编号 | 问题 | 位置 | 状态 |
|------|------|------|------|
| TD-F1 | G2pStep 成员遮蔽 | [G2pStep.h:47](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Core/G2pStep.h) + TagAndValidateStep.h | ✅ `f17ac89` |
| TD-F2 | Package.cpp goto 控制流 | [Package.cpp](file:///D:/projects/language-manager/core/lib/Package/Package.cpp) | ✅ `9b9da2d` |
| TD-F3 | PhonemeDict.cpp goto 控制流 | [PhonemeDict.cpp](file:///D:/projects/language-manager/core/lib/Support/PhonemeDict.cpp) | ✅ `bf48915` |
| TD-F4 | DependencyGraph const 方法修改状态 | [DependencyGraph.h:88-90](file:///D:/projects/language-manager/core/include/LangCore/Module/Dependency/DependencyGraph.h) | ✅ 已修复 |

### 4.2 P2 技术债（✅ 已完成）

| 编号 | 问题 | 状态 |
|------|------|------|
| TD-F5 | 5 个公共头文件 include 守卫命名不规范 | ✅ 已修复 |
| TD-F6 | 8 处 static_assert 消息误写 `LangPlugins::` | ✅ 已修复 |
| TD-F7 | include 路径风格混合 | ✅ 已修复 |
| TD-F8 | 4 处无参构造函数误用 explicit | ✅ 已修复 |
| TD-F9 | G2pRes 7 参数 legacy 构造 | ✅ 已 `[[deprecated]]` |
| TD-F10 | PhonemeDict 未禁止拷贝/移动 | ✅ `13308b8` |
| TD-F11 | DisplayText 本地化未实现 | ✅ `98ff863` |

### 4.3 Issues-Tracker 活跃问题

| 编号 | 问题 | 处理 |
|------|------|------|
| IT-1 | FormatStep::addSpaceBetweenPhones 命名与行为不符 | 重命名或文档说明 |
| IT-2 | LstmG2p V2 已完成样本继续参与解码 | 性能优化，低优先级 |
| IT-3 | `LANGPLUGINS_ENABLE_STATIC_PLUGINS` 选项未实现 | 文档标注"不可用"或移除选项 |

---

## 5. 文档与代码不一致

### 5.1 VoiceBank-Scoped-Package-Design.md §11 文本过期（✅ 已修复）

**问题**：文档头部声明 v3.1（保留 version 维度），但 §11 标题曾为"v4.0 变更：移除 ContextKey 版本维度"，正文描述与代码相反。

**修复**：✅ 已删除 §11 过期文本，文档版本回退为 v3.1。

### 5.2 Architecture-Overview.md §3 测试目录描述与实际不符（✅ 已修复）

**问题**：文档写 `tst_unit/`、`tst_context/`，实际目录是 `catch2/`、`tst_langCore/`。

**修复**：✅ 已更新为 `catch2/` + `common/` + `tst_langCore/`，Module-Reference.md §4 同步更新。

### 5.3 `PackageManager::open()` 文档未说明不解析传递依赖

**问题**：[PackageManager.h:58](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) `open()` 公共 API 未注释说明"不解析传递依赖"。

**修复**：见 [04-implementation-tasks.md](04-implementation-tasks.md) 任务 1.9。

---

## 6. v4.0 简化方案撤销决策记录

### 6.1 背景

本目录早期草案曾提出 **v4.0 简化设计**：移除 `ContextKey` 的 `version` 维度，仅保留 `context` 字符串隔离。其理由为"声库版本是宿主（ds-editor-lite）的概念，框架不需要感知"。

### 6.2 撤销依据

经与调用方 ds-editor-lite 实施代码双向核对，该前提**与实际数据流不符**，v4.0 方案予以撤销：

| 证据 | 来源 | 结论 |
|------|------|------|
| voicebank version 回退到 `singer.packageVersion()` | [synthrt 02-data-flow §2.1](file:///D:/projects/synthrt/docs/dspk-g2p-design/02-lite-singer-manager-analysis.md) | version 非空，被传给 `addPackagePath`/`G2pInput` |
| 测试用例 6/7 期望 `route.contextVersion.toString()=="1.2.0"` | [synthrt 03 §6.4](file:///D:/projects/synthrt/docs/dspk-g2p-design/03-lite-singerinfo-propagation.md) | version 是路由解析的必要组成 |
| `addPackagePath(context, version, paths)` 三参重载被调用 | [synthrt 01 §3.2](file:///D:/projects/synthrt/docs/dspk-g2p-design/01-editor-g2p-loading-flow.md) B5 | 版本化注册 API 被实际使用 |
| `G2pInput(lyric, g2pId, context, contextVersion)` 四参构造 | [synthrt 01 §3.4](file:///D:/projects/synthrt/docs/dspk-g2p-design/01-editor-g2p-loading-flow.md) D1 | version 进入推理链路 |
| `task("g2p", ctx, version, g2pId)` 四参重载被调用 | [synthrt 02 §3.4](file:///D:/projects/synthrt/docs/dspk-g2p-design/02-lite-singer-manager-analysis.md) | 版本化查找 API 被实际使用 |

### 6.3 撤销结论

- **保留** `ContextKey{context, version}` 结构体
- **保留** `G2pInput.contextVersion` / `G2pRes.contextVersion`
- **保留** `addPackagePath` / `setPackagePaths` / `task` / `tasks` 的版本化重载
- **保留** 版本化查找回退链（exact → unversioned fallback）
- **保留** `ModuleMetadata.contextVersion`
- **取消** 原 Phase 0（v4.0 简化重构）

> 该决策符合 [human-decisions.md ARCH-02](../decisions/human-decisions.md)（接口稳定，破坏公共头文件签名需递增 Level）与"稳定性优先"原则。调用方阶段 1-6 已实施 v3.0，无需返工。

---

## 7. 现状总结

LangCore 框架的**核心 context 隔离机制稳定可靠**：两阶段初始化、依赖解析回退、convert 不跨 context 回退均有代码实现 + catch2 单元测试覆盖。version 维度（v3.1）经调用方实施 + 12 测试用例验证为必需，v4.0 简化方案已正确撤销。

**已解决**：
- ✅ 技术债（§4）：全部 11 项 P1/P2 技术债已在 `f17ac89..c810e82` 修复完成
- ✅ 文档不一致（§5.1/§5.2）：§11 过期文本已删除，测试目录描述已修正

**本方案待处理**：
1. **一个关键漏洞**（ModelStep 不感知 context，S5 场景失败）—— 必须修复（synthrt O-2）
2. **架构债**（deprecated 方法暴露、失败 context 不可观测、initialize 幂等性）—— 需清理（synthrt O-1/O-5）
3. **文档不一致**（§5.3 `PackageManager::open()` 文档）—— 需修正
4. **跨模块契约**（ONNX driver 边界、启动期加载边界、三层校验）—— 需在框架侧显式声明（[03-host-integration-contract.md](03-host-integration-contract.md)）

本方案聚焦"漏洞修复 + 架构债清理 + 可观测性增强 + 跨模块契约明确"，不做架构重构，符合"不过度设计"原则。

---

**关联文档**: [README.md](README.md) · [02-interface-design.md](02-interface-design.md) · [03-host-integration-contract.md](03-host-integration-contract.md) · [04-implementation-tasks.md](04-implementation-tasks.md) · [05-design-principles-check.md](05-design-principles-check.md) · [decisions/human-decisions.md](../decisions/human-decisions.md)
