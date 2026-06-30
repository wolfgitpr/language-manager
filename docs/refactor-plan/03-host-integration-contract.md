# 03 — 宿主集成契约

> **日期**: 2026-07-01
> **状态**: 设计稿，待评审
> **目的**: 从框架侧视角显式声明 LangCore 与宿主（ds-editor-lite / synthrt）的集成契约，对齐 synthrt [05-cross-module-initialization-design](file:///D:/projects/synthrt/docs/dspk-g2p-design/05-cross-module-initialization-design.md) 的跨模块设计
> **来源**: 新写，对标 synthrt `01-editor-g2p-loading-flow` + `05-cross-module-initialization-design`

---

## 1. 本文档要解决的问题

synthrt [dspk-g2p-design](file:///D:/projects/synthrt/docs/dspk-g2p-design/README.md) 已从宿主侧定义了完整的跨模块初始化设计（ONNX driver 归属、启动期加载边界、三层依赖校验、SingerInfo 传递）。但 language-manager 框架侧此前未显式声明这些契约，导致：

| 缺口 | 表现 | 后果 |
|------|------|------|
| **G1 ONNX driver 边界不显式** | 框架无 `initDriver` API，driver 由宿主通过 `category("driver")->addObject` 注册，但未文档化 | 宿主不知道如何正确注册 driver；声库作者误以为可携带 onnxruntime.dll |
| **G2 启动期加载边界不显式** | `initialize()` 后 `addPackagePath` 语义无效是事实，但未作为硬约束声明 | 实现者可能误以为可绕过；用户期望运行期热加载 |
| **G3 稳定 API 白名单未声明** | 框架公共头文件多，宿主不知道哪些是稳定承诺、哪些是 deprecated | 宿主易误用 `checkDependencies` 等扁平化方法 |
| **G4 三层校验职责未划分** | 字段/路径/context/依赖图/driver 校验散落在不同模块 | 失败时职责推诿，难以定位 |

本文档从框架侧固定上述契约，作为 [01-current-state-audit.md](01-current-state-audit.md) §3 架构债的补充设计依据。若本文档与 synthrt 05 冲突，以 synthrt 05 为准（宿主侧视角权威）。

---

## 2. 框架与宿主职责边界

### 2.1 职责划分

```
┌─────────────────────────────────────────────────────────────┐
│  宿主（ds-editor-lite / synthrt）                             │
│                                                              │
│  · PackageManager 扫描安装目录、解析 dspk、构造 SingerInfo    │
│  · G2pRouteResolver 路由解析（纯函数，单点真相）              │
│  · LanguagePackageRegistrar 注册声库私有 context              │
│  · LaunchLanguageEngineTask 编排启动时序                      │
│  · GetPronunciationTask / G2pService 双链路 G2P 调用          │
│  · S2pMgr syllable→phoneme（独立于 LangCore）                │
│  · ONNX driver 注册（key 固定 "g2pOnnxDriver"）              │
│  · 失败 context 报告（通过 failedContexts()）                 │
├─────────────────────────────────────────────────────────────┤
│  LangCore 框架（本项目）                                      │
│                                                              │
│  · Manager::addPackagePath / setPackagePaths 包注册          │
│  · Manager::initialize 两阶段初始化 + 依赖解析               │
│  · Manager::convert context 隔离路由 + 批量转换              │
│  · Manager::task / tasks 版本化查找                          │
│  · ContextUtils context 名校验 + FQID 工具                   │
│  · PackageManager::open 包加载（不解析传递依赖）             │
│  · failedContexts() / contextState() 可观测性 API（新增）    │
│  · G2pRes::isOk() / isFailed() 便利方法（新增）              │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 模块间信息契约（对齐 synthrt 05 §3.2）

| 提供方 → 消费方 | 信息 | 形态 | 框架侧承诺 |
|----------------|------|------|-----------|
| 宿主 → LangCore::Manager | `addPackagePath(context, path)` | API 调用 | 必须在 `initialize()` 前（L-1） |
| 宿主 → LangCore::Manager | `addPackagePath(context, version, path)` | API 调用 | 必须在 `initialize()` 前；context 非空时 version 必非空（R-8） |
| 宿主 → LangCore::Manager | ONNX driver 对象（`g2pOnnxDriver`） | `category("driver")->addObject` | 必须在 `initialize()` 前（L-4） |
| LangCore::Manager → 宿主 | `convert()` 结果 `G2pRes` | 返回值 | 不跨 context 回退（C-6）；失败语义见 [02 §5](02-interface-design.md) |
| LangCore::Manager → 宿主 | `failedContexts()` 失败列表 | API 调用 | `initialize()` 后可查（[02 §4](02-interface-design.md)） |
| LangCore::ModuleSpec → 插件 | `contextKey()` | getter | `createModuleTask` 阶段注入（[02 §3](02-interface-design.md)） |

---

## 3. 稳定 API 白名单（框架侧承诺稳定）

### 3.1 稳定 API（宿主可依赖）

| API | 用途 | 稳定性承诺 |
|-----|------|-----------|
| `Manager::instance()` | 单例入口 | ✅ 签名稳定 |
| `Manager::addPackagePath(context, path)` | 注册官方默认包 | ✅ 签名稳定 |
| `Manager::addPackagePath(context, version, path)` | 注册声库版本化包 | ✅ 签名稳定 |
| `Manager::initialize()` | 一次性初始化 | ✅ 签名稳定（新增幂等防护，错误码 additive） |
| `Manager::convert(vector<G2pInput>)` | 批量 G2P 转换 | ✅ 签名稳定 |
| `Manager::task(category, ctx, id)` / `task(category, ctx, ver, id)` | 查找 task | ✅ 签名稳定 |
| `Manager::tasks(category, ctx)` / `tasks(category, ctx, ver)` | 列出 context 下所有 task | ✅ 签名稳定 |
| `Manager::contexts()` / `contextKeys()` | 枚举已注册 context | ✅ 签名稳定 |
| `Manager::contextState(ctxKey)` / `failedContexts()` | 可观测性（新增） | ✅ additive 稳定 |
| `ContextUtils::validateContextName` | context 名预校验 | ✅ 签名稳定 |
| `ContextUtils::formatFqid / parseFqid` | FQID 工具 | ✅ 签名稳定 |
| `G2pInput(lyric, g2pId, context, contextVersion)` | 输入数据结构 | ✅ 签名稳定 |
| `G2pRes`（8 参构造）+ `isOk()` / `isFailed()` | 输出数据结构 | ✅ 签名稳定（便利方法 additive） |

### 3.2 禁止使用（已 deprecated 或将私有化）

| API | 处理 | 见 |
|-----|------|----|
| `PackageManager::checkDependencies()` | 当前 Level（v3.x）追加 `[[deprecated]]` 警告；私有化推迟到 v4.x 并递增 Level | [02 §2](02-interface-design.md) |
| `PackageManager::getPackageInitializationOrder()` | 同上 | [02 §2](02-interface-design.md) |
| `PackageManager::loadPackagesInOrder()` | 同上 | [02 §2](02-interface-design.md) |
| `PackageManager::open(path)` 单独加载 | 保留 public，文档说明"不解析传递依赖" | [01 §3.3](01-current-state-audit.md) |
| `G2pRes` 7 参数 legacy 构造 | 已 `[[deprecated]]` | — |

> **v3 对齐修订**：原方案"直接 private 化"违反 ARCH-02"破坏性变更须递增 Level"，已修正为分两 Level 推进（v3.x 警告 + v4.x 私有化）。

### 3.3 ContextState 四态语义（v3 对齐修订）

`Manager::contextState(ctxKey)` / `failedContexts()` 返回的 `ContextState` 枚举为四态：

| 状态 | 含义 | 触发条件 |
|------|------|---------|
| `Pending` | 已注册但尚未初始化 | context 已 `addPackagePath` 注册，但 `initialize()` 尚未执行 |
| `Ready` | 初始化成功 | `initialize()` 后该 context 至少一个包加载成功 |
| `Failed` | 初始化失败 | 依赖缺失/环/Level 不兼容/driver 不可用导致该 context 所有包加载失败（不阻塞其他 context） |
| `NotRegistered` | 未注册 | 查询的 context 不在 `contexts()` 枚举中（`contextState()` 未命中返回此值，而非 `Pending`） |

> **与 SingerInfo.resolutionState 的分层语义**：SingerInfo 表达声库元数据解析状态（Resolved/Pending/Missing，宿主侧），ContextState 表达框架 context 生命周期（Pending/Ready/Failed/NotRegistered，框架侧），两者分层独立。

---

## 4. ONNX driver 边界（补齐 G1，对齐 synthrt D1/D2）

### 4.1 现状核实（框架侧代码）

| 事实 | 证据 |
|------|------|
| 框架无 `initDriver` / `registerDriver` 公共 API | `Manager.h`/`PackageManager.h` 无此方法 |
| ONNX driver 是 `LangCore::SessionFactory` 子类 | `OnnxDriver.h`：`class OnnxDriver : public SessionFactory` |
| driver 插件 key 固定为 `"onnx"` | `OnnxDriver/main.cpp`：`LANGCORE_DEFINE_DRIVER_PLUGIN(..., "onnx", 1)` |
| driver 注册名固定为 `"g2pOnnxDriver"`（全局裸名） | `tests/tst_langCore/main.cpp:78` |
| LstmG2p 用裸名查找 driver | `LstmG2p/internal/TaskImplBase.cpp:72`：`getFirstObject("g2pOnnxDriver")` |
| driver 不纳入 context 隔离 | 裸名无 context 前缀，所有 context 共享 |
| driver 不纳入 package.json 依赖声明 | `DependencyResolver` 不检查 driver |
| driver 缺失时 LstmG2p 静默降级 | `TaskImplBase.cpp:80`：`m_driverAvailable=false`，`start()` 返回 `DriverUnavailable` |

### 4.2 框架侧契约（D1）

**ONNX driver 是宿主侧全局基础设施**：

- 框架不提供 driver 注册 API；driver 通过 `Manager::category("driver")->addObject("g2pOnnxDriver", driverObj)` 注册
- driver **不纳入 context 隔离**（裸名，所有 context 共享）
- driver **不纳入 package.json 依赖声明**（`DependencyResolver` 不检查 driver）
- 声库 G2P 子包**不得**携带 `onnxruntime.dll`（安全：避免任意库加载；体积：单例共享）
- 声库 G2P 子包**不声明**对 driver 的依赖
- 声库 G2P 子包若使用 ONNX-based class（如 `g2p.model.LstmG2pInference`），**隐式假设**宿主已注册 `"g2pOnnxDriver"`

### 4.3 宿主注册时序约束（D2 / L-4 硬约束）

**宿主必须在 `langMgr->initialize()` 之前完成 ONNX driver 注册**：

```
宿主启动序列（不可乱序）:
  1. langMgr->addPluginPath("org.openvpi.Driver", driversDir)   // 注册 driver 插件路径
  2. langMgr->addPluginPath("org.openvpi.Task", g2psDir)         // 注册 task 插件路径
  3. langMgr->addPackagePath("", officialG2pDir)                 // 官方默认 G2P 包
  4. [等 PackageManager 就绪] waitForPackageModuleReady()
  5. LanguagePackageRegistrar::registerAll()                     // 声库私有 G2P context
  6. initializeOnnxDriver(langMgr, ep, deviceIndex)              // ★ 必须在 initialize() 前
       ├─ onnxPlugin = langMgr->plugin<DriverPlugin>("onnx")
       ├─ onnxDriver = onnxPlugin->create()
       ├─ onnxDriver->initialize({ep, deviceIndex, runtimePath, loadFromProcess})
       └─ langMgr->category("driver")->addObject("g2pOnnxDriver", onnxDriver)
  7. langMgr->initialize()                                       // 此后 task.initialize() 可查到 driver
```

**为何步骤 6 必须在步骤 7 之前**：`langMgr->initialize()` 内部逐包 `createModuleTask()` 会触发每个 G2P task 的 `initialize()`，其中 LstmG2p 的 `TaskImplBase::initialize()` 会执行 `getFirstObject("g2pOnnxDriver")` 查找 driver。若 driver 未注册，task 进入静默降级态（`m_driverAvailable=false`），后续 `start()` 返回 `DriverUnavailable`，但 `initialize()` 本身返回成功——这是**当前框架的静默降级缺陷**。

### 4.4 框架侧不前移 driver 校验（对齐 synthrt O-3 决策）

synthrt O-3 建议"在 `Manager::initialize()` Phase 3 对 ONNX-based class 的 task 增加 driver 可用性检查"。本方案**不采纳**框架侧前移，理由见 [02 §4.5](02-interface-design.md)：

1. 宿主侧 `qFatal` 断言（synthrt 05 §6.5 对策 1）已提供硬保护
2. 框架侧前移需识别"ONNX-based class"清单，违反插件职责单一（[ARCH-01](../decisions/human-decisions.md)）
3. `failedContexts()`（[02 §4](02-interface-design.md)）已能覆盖 driver 缺失导致的 task 失败

### 4.5 ExecutionProvider 来源

| EP | 决策 | 理由 |
|----|------|------|
| `CPUExecutionProvider` | 默认 | 所有平台可用 |
| `CUDAExecutionProvider` | 宿主选项 | 用户配置；声库不可指定 |
| `DMLExecutionProvider` | 宿主选项 | Windows 用户配置 |

**声库 G2P 子包不可指定 EP**：EP 是进程级硬件决策，由宿主根据用户配置统一设定。

---

## 5. 启动期加载边界（补齐 G2，对齐 synthrt D3 / L-1~L-4）

### 5.1 启动期定义

```
T_start  应用启动
  │
  ├─ 声库扫描（宿主 PackageManager 异步）
  ├─ 语言引擎启动任务（LaunchLanguageEngineTask）
  │     ├─ addPluginPath / addPackagePath(官方)
  │     ├─ waitForPackageModuleReady
  │     ├─ registerAll (声库私有 context)        ◀── 自定义 G2P 注册窗口
  │     ├─ initializeOnnxDriver
  │     └─ langMgr->initialize()                 ◀── 窗口关闭
  │
  ├─ 项目加载（dspx）
  └─ 用户交互
T_end    应用退出
```

**自定义 G2P 注册窗口**：从应用启动到 `langMgr->initialize()` 调用前。此窗口内 `addPackagePath` 有效；此后语义无效（框架无 reload，路径虽可加入 `contextPackagePaths` 但不会触发重新扫描）。

### 5.2 硬约束（对齐 synthrt 05 §5.2）

| 约束 | 说明 | 违反后果 | 框架侧处理 |
|------|------|---------|-----------|
| **L-1** 自定义 G2P 仅允许启动期加载 | `addPackagePath` 仅在 `initialize()` 前有效 | 运行期新装声库的 G2P 不可用 | 框架不报错（语义无效），宿主侧提示重启 |
| **L-2** `initialize()` 不可重复调用 | 二次调用重跑全流程（非增量），可能导致已加载 task 状态不一致 | task 状态不一致 | [02 §6](02-interface-design.md) 幂等防护，返回 `AlreadyInitialized` |
| **L-3** 运行期检测新 G2P 声库 → 提示重启 | 宿主侧 `newG2pVoicebankDetected` 信号 → UI Toast | 用户须重启应用使新 G2P 生效 | 框架不参与检测（宿主侧职责；v3 对齐：宿主侧已删除此信号，框架明确不支持 reload，运行期新装声库 G2P 不可用） |
| **L-4** 官方默认 context 必须先于声库 context 注册 | `addPackagePath("", officialDir)` 在 `registerAll()` 前 | 声库 G2P 依赖官方模块时回退失败 | 框架 R-8 校验：默认 context 不允许带 version |

### 5.3 运行期新增声库的处理（非热加载，对齐 synthrt D3）

> **v3 对齐修订**：`newG2pVoicebankDetected` 是宿主侧信号，框架不参与；v3 对齐中宿主侧已删除此信号。框架明确不支持 reload，运行期新装声库的 G2P 不可用。`enableVoicebankG2p` 防御开关是宿主侧概念，框架文档不涉及。

```
运行期用户安装新声库（含 G2P）:
  ├─ 宿主 PackageManager::refreshInstalledPackages()
  ├─ 扫描到新声库有 g2pPackagePaths
  ├─ 对比 m_knownG2pSingerIds 快照
  ├─ [宿主侧] emit newG2pVoicebankDetected(singerNames)   // 宿主侧信号，框架不参与
  │     注：v3 对齐中宿主侧已删除此信号；框架明确不支持 reload
  └─ UI: Toast("新声库 '%1' 含自定义 G2P，需重启应用生效")

  ⚠️ 不调用 langMgr->addPackagePath（无效）
  ⚠️ 不调用 langMgr->initialize()（不可重复，L-2）
  ⚠️ 该声库的 G2P 在本次运行不可用，推理走 copy fallback / 填词走官方回退
```

---

## 6. 三层依赖校验（补齐 G3，对齐 synthrt D4）

### 6.1 三层校验模型（对齐 synthrt 05 §6.1）

```
┌─────────────────────────────────────────────────────────────┐
│  层 1: dspk 解析期（宿主 SingerProvider + PackageManager）    │
│  时机: srt::SynthUnit.open(dspk) → parseSpec → loadSpec      │
│  范围: 字段存在性/类型/路径有效性                                │
│  失败: 该声库记入 failedPackages，不阻塞其他声库               │
│  职责: 宿主侧                                                 │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  层 2: context 注册期（宿主 LanguagePackageRegistrar）         │
│  时机: registerAll() 内，addPackagePath 调用前                │
│  范围: context 名/version/去重/路径存在                        │
│  失败: 该语言项跳过（qWarning），不阻塞其他语言                 │
│  职责: 宿主侧（调用 ContextUtils::validateContextName）       │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  层 3: initialize 期（LangCore::Manager::initialize）         │
│  时机: langMgr->initialize() 内部                              │
│  范围: 模块依赖图/Level 兼容/环检测/ONNX driver 可用性         │
│  失败: 默认 context 失败=致命(Ord-1)；声库 context 失败=隔离(Ord-2) │
│  职责: 框架侧                                                 │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 层 3 校验清单（框架侧职责，对齐 synthrt 05 §6.4）

由 `LangCore::Manager::initialize()` 内部完成，宿主无需干预，但须理解失败语义。

| # | 校验项 | 失败处理 | 框架实现 |
|---|--------|---------|---------|
| 3.1 | 默认 context 模块依赖图无缺失（Dep-1） | `initialize()` 返回 Error，致命（Ord-1） | Manager.cpp Phase 1 |
| 3.2 | 默认 context 无环（Dep-6） | `initialize()` 返回 Error，致命（Ord-1） | findCycles |
| 3.3 | Level 兼容性（ARCH-02） | `initialize()` 返回 Error | LevelCompatibilityChecker |
| 3.4 | 声库 context 依赖图无缺失（Dep-1，可在默认 context 回退 Dep-2） | 该 context 标 Failed，不阻塞其他（Ord-2） | Manager.cpp Phase 2 |
| 3.5 | 声库 context 无环 | 该 context 标 Failed | findCycles |
| 3.6 | ONNX driver 可用性（ONNX-based class 需要） | task 静默降级（`m_driverAvailable=false`） | TaskImplBase.cpp |

### 6.3 校验失败的全局行为矩阵（对齐 synthrt 05 §6.6）

| 失败层 | 失败范围 | 该声库 G2P | 其他声库 | 宿主整体 |
|--------|---------|-----------|---------|---------|
| 层 1 | 单声库字段错误 | 该声库不入 successfulPackages | 不影响 | 继续 |
| 层 2 | 单语言 context 注册失败 | 该语言走官方/invalid | 不影响 | 继续 |
| 层 3.1/3.2/3.3 | 默认 context 失败 | 全部 G2P 不可用 | 全部不可用 | `initialize()` 返回 Error，宿主应阻断 G2P 依赖功能 |
| 层 3.4/3.5 | 单声库 context 失败 | 该声库 G2P 走官方回退 | 不影响 | 继续，记录 failedContexts |
| 层 3.6 | ONNX driver 缺失 | ONNX-based task 降级 | 不影响 | qFatal（宿主侧断言） |

### 6.4 失败 context 反馈（对齐 synthrt D8 / O-1）

层 3.4/3.5 失败的 context 通过 [02 §4](02-interface-design.md) `failedContexts()` API 反馈给宿主。宿主在 `initialize()` 后枚举失败 context，向用户报告"声库 X 的 G2P 初始化失败，已回退官方"。

---

## 7. 两阶段初始化与失败语义（框架侧实现，对齐 synthrt 05 §9 B7）

### 7.1 两阶段初始化（Ord-1 / Ord-2）

`Manager::initialize()` 内部分两阶段（[Manager.cpp:61-252](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp)）：

```
Phase 1: 默认 context (Ord-1, 致命)
  ├─ 加载默认 context 的所有包
  ├─ 依赖图校验（层 3.1/3.2/3.3）
  ├─ createModuleTask 创建 task
  └─ 任一失败 → initialize() 返回 Error（致命，宿主应阻断 G2P 依赖功能）

Phase 2: 声库 context (Ord-2, 隔离失败)
  ├─ 遍历每个声库 context
  ├─ 加载该 context 的所有包
  ├─ 依赖图校验（层 3.4/3.5，可回退默认 context Dep-2）
  ├─ createModuleTask 创建 task（注入 contextKey，见 [02 §3](02-interface-design.md)）
  └─ 单个 context 失败 → 标 Failed，继续下一个 context（不阻塞）

Phase 3: 任务索引
  └─ 构建 g2p/dict category 的 task 索引（供 task()/tasks() 查询）
```

### 7.2 失败语义

| 阶段 | 失败范围 | `initialize()` 返回 | `failedContexts()` | 宿主行为 |
|------|---------|---------------------|-------------------|---------|
| Phase 1 | 默认 context | `Error`（致命） | 不含默认 context | 阻断 G2P 依赖功能 |
| Phase 2 | 单个声库 context | `Success` | 含该 context | qWarning + 继续 |
| Phase 2 | 多个声库 context | `Success` | 含所有失败 context | 逐个 qWarning + 继续 |

### 7.3 宿主侧调用时序（对齐 synthrt 05 §9 B7-B8）

```
B7. langMgr->initialize()                      (L-2 幂等防护，窗口关闭 L-1)
      ├─ Phase 1: 默认 context (Ord-1, 致命)
      ├─ Phase 2: 声库 context (Ord-2, 隔离失败)
      └─ Phase 3: 任务索引
B8. 枚举 failedContexts()                      (synthrt O-1 必需 API, D8 错误反馈)
      └─ UI 报告失败的声库 G2P
```

---

## 8. S2P 独立性（对齐 synthrt 01 §2 / 03 §9）

**S2P（syllable→phoneme）独立于 LangCore G2P**：

| 模块 | 输入 | 输出 | 路由依据 | 归属 |
|------|------|------|---------|------|
| LangCore G2P | lyric（文本） | syllable（音节） | `(context, version, g2pId)` | 框架侧 |
| S2pMgr S2P | syllable（音节） | phonemes（音素序列） | `(singerId, g2pId)` | 宿主侧 |

- LangCore 框架**不含 S2P**
- S2P 不调 `LangCore::Manager`，基于 `PhonemeConverter` 库
- S2P 按 `(singerId, g2pId)` 注册，无 context 概念
- SingerInfo 的 `s2pMode/s2pFile/onsetMode/onsetFile` 字段由宿主侧消费，框架不解析

---

## 9. 设计原则核对

| 原则（[human-decisions.md](../decisions/human-decisions.md)） | 本文档遵守情况 |
|------|--------------|
| 单一职责、模块化 | 模块间信息契约明确（§2.2），ONNX driver 归宿明确（§4） |
| 接口稳定、Level 锚定 | 不修改 Level 1 既有 API；新增 API 为 additive |
| 依赖抽象、构造注入 | 宿主通过 SingerInfo 契约注入；框架通过 `addPackagePath` 接收 |
| Result/Expected 错误传播 | 三层校验均返回结构化错误/警告，不抛异常（[ROBUST-02](../decisions/human-decisions.md)） |
| 不抛异常（业务层） | 校验失败 qWarning/qCritical/qFatal，不 throw |
| Open/Closed | 新增 failedContexts/contextState API，不修改现有路由核心 |
| 不引入新外部依赖 | ONNX driver 由现有 LangCore 插件系统提供，无新依赖 |
| 不过度设计 | 不引入 driver 依赖声明（框架不支持）；不在框架侧前移 driver 校验 |
| 启动期加载边界明确 | L-1~L-4 硬约束 + 窗口定义（§5） |
| 完整依赖校验 | 三层校验清单（§6）覆盖字段/路径/context/依赖图/driver |

---

**关联文档**: [README.md](README.md) · [01-current-state-audit.md](01-current-state-audit.md) · [02-interface-design.md](02-interface-design.md) · [04-implementation-tasks.md](04-implementation-tasks.md) · [05-design-principles-check.md](05-design-principles-check.md) · [decisions/human-decisions.md](../decisions/human-decisions.md) · [synthrt 01 编辑器通用流程](file:///D:/projects/synthrt/docs/dspk-g2p-design/01-editor-g2p-loading-flow.md) · [synthrt 05 跨模块初始化设计](file:///D:/projects/synthrt/docs/dspk-g2p-design/05-cross-module-initialization-design.md)
