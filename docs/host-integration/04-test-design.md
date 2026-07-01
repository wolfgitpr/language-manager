# 04 · 测试设计

本文档定义 LangCore 宿主集成相关的测试设计，覆盖三大测试领域，采用双层测试策略。这是本目录最核心的文档，包含具体的测试用例描述与实现计划。

> 2026-07-02 修订：修正 I-C2/I-C3 用例预期以匹配实际框架行为（`addPackagePath` 静默注册但永不被处理，非 no-op；`initialize()` 成功后第二次返回 `Error::AlreadyInitialized`，非幂等 no-op）。详见 [01-framework-capabilities.md](01-framework-capabilities.md) §1.2/§1.3 与 [03-host-integration-contract.md](03-host-integration-contract.md) §3。

## 1. 测试策略：双层架构

采用 **双层测试策略**，兼顾速度与真实度：

### 1.1 第一层：Catch2 单元测试（快速、mock、无 ONNX 依赖）

| 属性 | 说明 |
| --- | --- |
| 框架 | Catch2 v2.13.10（单头文件，`tests/common/catch.hpp`） |
| 位置 | `tests/catch2/` |
| 可执行文件 | `LangMgrTest`（链接 `LangCore::LangCore`） |
| 依赖 | mock `ModuleMetadata` + mock `DependencyResolver`，**不依赖真实 ONNX** |
| 目标 | 模拟路由解析 + 回退逻辑、初始化约束、FQID 两级查找 |
| 参考模式 | `tst_context_isolation.cpp`（已用 mock ModuleMetadata + DependencyResolver） |

### 1.2 第二层：tst_langCore 增强（真实 ONNX、端到端）

| 属性 | 说明 |
| --- | --- |
| 框架 | 非 Catch2，纯 `main()` + `std::cout`（待增强为断言宏） |
| 位置 | `tests/tst_langCore/main.cpp` |
| 可执行文件 | 独立可执行文件 |
| 依赖 | 真实 ONNX 驱动（降级模式）、真实 G2P 包 |
| 目标 | 模拟 `ds-editor-lite` 实际调用流程，多上下文端到端 |

### 1.3 构建与运行

| 项 | 命令 / 说明 |
| --- | --- |
| 构建标志 | `-DLANGMGR_BUILD_TESTS=ON`（必需）；`-DLANGMGR_BUILD_PLUGINS=ON`（tst_langCore 需要） |
| 运行 | `ctest --test-dir build`，或直接运行 `LangMgrTest.exe` |

## 2. 测试领域

按用户决策，聚焦 **三大测试领域**，每个领域同时覆盖 Catch2（mock）与 tst_langCore（端到端）两层。

---

### 测试领域 1：S5 多上下文端到端

**目标**：验证 ModelStep FQID 两级查找（决策 D4）在多上下文场景下的正确性，以及 `ToOfficial` / `Never` 回退策略。

#### 2.1.1 Catch2 层：ModelStep FQID 两级查找（mock 模块）

**测试文件**：扩展 `tests/catch2/tst_context_isolation.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| S5-C1 | `g2pId` 仅在默认上下文存在，私有上下文不存在 | 回退默认上下文，转换成功 |
| S5-C2 | `g2pId` 仅在私有上下文存在 | 使用私有上下文模块，转换成功 |
| S5-C3 | `g2pId` 在两个上下文都存在 | 优先使用私有上下文模块（覆盖） |
| S5-C4 | `g2pId` 在两个上下文都不存在 | 转换失败，`errorType != NoError` |

**实现要点**：
- 使用 mock `ModuleMetadata` 构造默认上下文与私有上下文的模块集合。
- 使用 mock `DependencyResolver` 控制依赖解析结果。
- 断言 `G2pRes::isOk()` / `isFailed()`，并校验 `g2pContext` 字段指向正确上下文（D10：原 `context`）。
- 参考 `tst_context_isolation.cpp` 现有的 mock 模式。

#### 2.1.2 tst_langCore 层：真实多上下文端到端

**测试文件**：增强 `tests/tst_langCore/main.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| S5-E1 | 默认上下文注册官方 G2P 包 + 私有上下文注册自定义 G2P 包；用私有上下文转换 | 使用自定义 G2P，结果与自定义包一致 |
| S5-E2 | 私有上下文转换失败（自定义 G2P 报错）→ `ToOfficial` 回退 | 回退到官方上下文，转换成功 |
| S5-E3 | 私有上下文转换失败 → `Never` 策略 | 不回退官方，复制 fallback（lyric） |

**实现要点**：
- 在默认上下文（`context=""`）注册官方 G2P 包。
- 在私有上下文（`context="SingerA"`）注册自定义 G2P 包。
- S5-E2：构造自定义 G2P 必失败的场景（如损坏的模型），验证 `G2pConvertRunner::convert(..., ToOfficial)` 的回退。
- S5-E3：同样失败场景，验证 `G2pConvertRunner::convert(..., Never)` 直接复制 fallback。
- 模拟 `ds-editor-lite` 的 `G2pConvertRunner` 流程：主转换 → 检测 `isFailed()` → 按策略回退或不回退。
- **g2pSource 字段验证（D10）**：S5-E1 私有上下文转换成功时，断言 `G2pRes.g2pSource == "voicebank"`；对照默认上下文转换结果 `g2pSource == "official"`。失败路径（S5-E2/E3）同样需断言 `g2pSource` 与 `g2pContext` 一致（来源由输入参数 `G2pInput.g2pContext` 判定，非由成败判定）。

---

### 测试领域 2：路由两级决策

**目标**：验证 `G2pRouteResolver` 的两级路由逻辑（声库上下文 vs 官方上下文），以及路由无效的判定条件。

#### 2.2.1 Catch2 层：模拟 G2pRouteResolver 逻辑（mock）

**测试文件**：新建 `tests/catch2/tst_routing_decision.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| R-C1 | 声库 `g2pPackagePaths` 非空 | `context = singerId`，`source = voicebank` |
| R-C2 | 声库 `g2pPackagePaths` 为空 | `context = ""`，`source = official` |
| R-C3 | `resolutionState = Pending` | 路由无效 |
| R-C4 | `resolutionState = Missing` | 路由无效 |
| R-C5 | 语言未找到 | 路由无效 |
| R-C6 | `g2pId` 为空或未知 | 路由无效 |

**实现要点**：
- 用 mock 构造 `singerInfo`，控制 `g2pPackagePaths` 是否为空。
- 用 mock 构造 `resolutionState`（Pending / Missing / Ready）。
- 断言路由结果的 `context`、`source` 字段，或断言路由无效（返回无效路由对象）。
- 此测试不依赖真实 ONNX，纯逻辑判定。

#### 2.2.2 tst_langCore 层：真实包路由决策验证

**测试文件**：增强 `tests/tst_langCore/main.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| R-E1 | 注册声库私有上下文 + 自定义 G2P；转换 | 使用私有上下文，结果为自定义 G2P 输出 |
| R-E2 | 移除自定义 G2P（仅官方上下文）；转换 | 回退到官方上下文，结果为官方 G2P 输出 |

**实现要点**：
- R-E1：`addPackagePath("SingerA", customPath)` 后转换，验证 `G2pRes.g2pContext == "SingerA"`。
- R-E2：模拟「移除」可通过重启进程时不注册私有上下文实现（受 L-3 约束，运行时无法移除）。
- 验证 `G2pRes.g2pContext` 与 `g2pId` 字段指向预期的上下文与模块。

---

### 测试领域 3：启动时序与初始化

**目标**：验证 `Manager` 的初始化约束（L-1～L-4）与幂等性。

#### 2.3.1 Catch2 层：Manager 初始化约束（mock）

**测试文件**：新建 `tests/catch2/tst_init_constraints.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| I-C1 | `addPackagePath` 在 `initialize()` 之前调用 | 注册成功，上下文进入 `Pending` → `Ready` |
| I-C2 | `addPackagePath` 在 `initialize()` 之后调用 | 路径静默注册，但**永不被处理**，上下文停留 `Pending` 直至进程结束 |
| I-C3 | `initialize()` 调用两次 | 第一次成功；第二次返回 `Error::AlreadyInitialized`（success-gated 幂等，非静默 no-op） |
| I-C4 | `initialized()` 在 `initialize()` 之前 | 返回 `false` |
| I-C5 | `initialized()` 在 `initialize()` 之后 | 返回 `true` |

**实现要点**：
- I-C1：`addPackagePath` → `initialize()` → `ContextState == Ready`。
- I-C2：`initialize()` 成功 → `addPackagePath` → 路径已被加入内部列表，但 `initialize()` 不可重跑（L-2），新路径永不进入扫描/依赖解析流程；验证上下文状态为 `Pending`（已注册未处理），非 `NotRegistered`。
- I-C3：连续两次 `initialize()`，第一次返回成功；第二次断言返回 `Error{Error::AlreadyInitialized}`（依据 `Manager.cpp:61-68` 的 `impl.initialized` 守卫，**不**是 no-op）。
- I-C4 / I-C5：调用 `initialized()` 断言返回值。
- 依据 `Manager.cpp:61-68` 的 success-gated 幂等守卫设计。

#### 2.3.2 tst_langCore 层：完整启动序列模拟

**测试文件**：增强 `tests/tst_langCore/main.cpp`

| 用例编号 | 场景 | 预期 |
| --- | --- | --- |
| I-E1 | 按 `LaunchLanguageEngineTask` 模式执行完整启动序列 | 所有步骤成功，`convert` 可用 |
| I-E2 | 启动序列完成后调用 `convert` | 转换成功，返回有效 `G2pRes` |

**实现要点**：
- 完整序列：`addPluginPath` → `addPackagePath(official)` → `addPackagePath(voicebank)` → `initializeOnnxDriver` → `initialize`。
- 验证每步无错误（`initialize()` 后 `initialized() == true`）。
- 验证 `convert(inputs)` 返回 `isOk()` 结果。

## 3. 实现计划

### 3.1 新增 Catch2 测试文件

| 文件 | 覆盖领域 | 状态 |
| --- | --- | --- |
| `tests/catch2/tst_routing_decision.cpp` | 测试领域 2（路由两级决策，mock） | ✅ L1 已完成（R-C1~C6） |
| `tests/catch2/tst_init_constraints.cpp` | 测试领域 3（启动时序，mock） | ✅ L1 已完成（I-C4 + 失败路径 + 状态转换） |
| `tests/catch2/tst_context_isolation.cpp` | 测试领域 1 Catch2 部分（S5 FQID 查找） | ✅ L1 已完成（S5-C1~C6） |

**L1 已实现测试用例汇总**（409 assertions，全部通过）：

| 文件 | 用例 | 说明 |
| --- | --- | --- |
| `tst_context_isolation.cpp` | S5-C1~C6 | C1 默认回退 / C2 私有命中 / C3 私有优先 / C4 双 miss / C5 默认无回退分支 / C6 版本化 context |
| `tst_init_constraints.cpp` | ic4 / ic2-failed / ic-retry / ic-noDefault / ic-emptyDefault / ic-stateTrans / ic-failedLeavesPending / ic-failedContexts | L-1~L-4 约束 + 幂等防护（成功路径 I-C1/C3/C5 需 L3） |
| `tst_routing_decision.cpp` | rc1~rc6 | 声库路由 / 官方回退 / 版本化 packagePaths / FQID 构造 / contexts 列举（G2pRouteResolver 在宿主侧，本测试验证 LangCore 路由原语） |

> **L1 vs L3 边界**：L1 测试不依赖真实 ONNX/G2P 包，仅验证 LangCore 原语（ObjectPool FQID 查找、PackageManager 路由原语、Manager 初始化约束的失败路径）。以下需 L3（`tst_langCore`，真实包 + ONNX）：
> - I-C1：`addPackagePath` → `initialize()` 成功 → `Ready`
> - I-C3：成功 `initialize()` 后第二次调用返回 `AlreadyInitialized`
> - I-C5：成功 `initialize()` 后 `initialized() == true`
> - S5-E1~E3 / R-E1~E2 / I-E1~E2：端到端转换与回退策略

### 3.2 tst_langCore 增强

| 增强项 | 说明 |
| --- | --- |
| 添加断言宏 | 从 `cout` 输出改为断言（如 `REQUIRE` 风格或自定义 `ASSERT_` 宏） |
| 移除硬编码路径 | 第 93、117 行 `"D:\projects\language-manager\res\G2pPackages"` 改为相对路径或环境变量 |
| 多上下文场景 | 新增测试领域 1 + 2 + 3 的端到端用例 |
| 模拟 G2pConvertRunner | 实现 `ToOfficial` + `Never` 两种策略的转换流程 |

### 3.3 tst_langCore 现有问题修复

| 问题 | 位置 | 修复方案 |
| --- | --- | --- |
| 硬编码绝对路径 | `main.cpp` 第 93、117 行 | 改用相对路径（基于源码目录）或环境变量 `LANGMGR_RES_DIR` |
| 无断言宏 | 全文 `std::cout` | 引入断言宏，失败时返回非零退出码 |
| 无多上下文 | 仅单上下文 | 增加私有上下文注册与转换 |

## 4. 测试文件结构

```
tests/
├── catch2/
│   ├── tst_context_isolation.cpp   (扩展：S5 FQID 两级查找测试)
│   ├── tst_routing_decision.cpp    (新建：路由两级决策)
│   ├── tst_init_constraints.cpp    (新建：启动时序/初始化约束)
│   ├── tst_context_convert.cpp     (已有：g2pContext 字段校验)
│   ├── tst_context_dedup.cpp       (已有：模块去重)
│   ├── tst_context_version.cpp     (已有：版本化上下文)
│   ├── tst_context_validation.cpp  (已有：上下文名校验)
│   └── tst_fqid.cpp                (已有：FQID 解析)
└── tst_langCore/
    └── main.cpp                    (增强：多上下文 + 断言 + 去硬编码)
```

## 5. 参考：ds-editor-lite 调用流程（测试模拟依据）

测试需精确模拟 `ds-editor-lite` 的以下调用流程。

### 5.1 FillLyric 路径（填词）

```
1. TextTagger::tag(text)                     ← 语言检测
        │
        ▼
2. G2pRouteResolver::resolve(singerInfo, language)   ← 两级路由
   ├─ g2pPackagePaths 非空 → context = singerId, source = voicebank
   └─ g2pPackagePaths 为空 → context = "",        source = official
        │
        ▼
3. G2pService::convert()                     ← 4 级回退
   (langToRoute → LangNote.g2pId → langToG2pId → 官方命名)
        │
        ▼
4. G2pConvertRunner::convert(mgr, requests, G2pFallbackPolicy::ToOfficial)
   ├─ 主转换：mgr->convert(inputs)  [私有上下文]
   └─ 回退：若 isFailed() && context 非空 → mgr->convert(inputs) [官方上下文]
        │
        ▼
5. 结果映射到 G2pResult
```

### 5.2 Inference 路径（推理，GetPronunciationTask）

```
1. 逐音符：G2pRouteResolver::resolve(m_singerInfo, note.language)   ← 两级路由
        │
        ▼
2. 若路由无效 → 复制 fallback (lyric)，绝不官方回退
        │
        ▼
3. G2pConvertRunner::convert(langMgr, requests, G2pFallbackPolicy::Never)
   └─ 仅主转换：mgr->convert(inputs)  [私有上下文]
        │
        ▼
4. 结果映射到 pronunciation 字符串
```

### 5.3 共同点

- 两条路径的核心 LangCore API 都是 `mgr->convert(inputs)`。
- `ToOfficial`：主转换 → 检测失败 → 用官方上下文回退转换。
- `Never`：仅主转换 → 失败时复制 fallback，不回退官方。
- 路由两级决策（`G2pRouteResolver`）决定 `convert` 使用的 `g2pContext`（映射到 `G2pInput.g2pContext`，D10）。

## 6. 测试用例与代码事实映射

| 测试用例 | 对应代码事实 | 验证点 |
| --- | --- | --- |
| S5-C1～C4 | `ModelStep.cpp:53-60`（两级查找） | FQID 回退正确性 |
| S5-E1～E3 | `G2pConvertRunner` ToOfficial/Never | 端到端回退策略 |
| R-C1～C6 | `G2pRouteResolver::resolve` 两级路由 | 路由决策与无效判定 |
| R-E1～E2 | `convert` 的 `g2pContext` 字段 | 真实包路由 |
| I-C1～C5 | `Manager.cpp:61-68`（success-gated 幂等守卫）、`Manager.h:27`（`initialized()`）、`PackageManager.cpp:552-575`（`addPackagePath` 不检查 `initialized`） | 初始化约束（L-1/L-2/L-3） |
| I-E1～E2 | 完整启动序列 | 端到端初始化 |

## 7. 源码引用

- ModelStep S5 修复：`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
- Manager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`
- Manager 实现（幂等守卫）：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`
- 公共类型：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`
- 上下文隔离测试（mock 模式参考）：`file:///D:/projects/language-manager/tests/catch2/tst_context_isolation.cpp`
- 端到端测试（待增强）：`file:///D:/projects/language-manager/tests/tst_langCore/main.cpp`
- Catch2 单头文件：`file:///D:/projects/language-manager/tests/common/catch.hpp`
