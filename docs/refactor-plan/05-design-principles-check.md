# 05 — 设计原则核对

> **日期**: 2026-07-01
> **核对对象**: [refactor-plan/](.) 全部 5 篇文档（README + 01-04）
> **对照基准**: [decisions/human-decisions.md](../decisions/human-decisions.md) ARCH / ROBUST / INFRA / PACK 四章共 17 条原则
> **目的**: 在实施前验证方案合规性，识别冲突并提出消解措施；本文件作为实施前的"放行检查单"

---

## 0. 核对方法

对每条原则执行：
1. **要求摘要**：从 human-decisions.md 提取关键约束
2. **方案契合度**：本方案相关设计是否合规（✅ 合规 / ⚠️ 需注意 / ❌ 违反）
3. **证据/页码**：方案文档中的对应章节
4. **行动项**：若需注意或违反，列出具体行动

---

## 第一章：ARCH — 架构与模块设计

### ARCH-01：插件职责单一

- **要求**：插件只做一件事；遇错直接返回，不内部重试/回滚；fallback 由 ChainG2p 流水线层协调
- **契合度**：✅ 合规
- **证据**：
  - [02 §3](02-interface-design.md) ModelStep 修复后仍只做"查 task → 批量推理 → 映射结果"一件事；FQID 查找失败时禁用模型推理（`m_enabled = false`），由 ChainG2p 上层 FallbackStep 处理
  - [02 §4](02-interface-design.md) `failedContexts()` 为 Manager 级 API，不污染插件层
- **行动项**：无

### ARCH-02：接口稳定，Level 锚定兼容性

- **要求**：破坏性 API 变更必须递增 Level；推荐 Version 首位 = Level
- **契合度**：⚠️ 需注意
- **证据**：
  - [02 §1.2](02-interface-design.md) 新增 API 均为 additive（新增 getter / 方法 / 错误码），不需递增 Level ✅
  - [02 §2](02-interface-design.md) 私有化 `checkDependencies` / `getPackageInitializationOrder` / `loadPackagesInOrder` — **可能破坏源码兼容** ⚠️
- **冲突点**：将 public 方法移至 private 会破坏已调用这些方法的宿主代码的源码兼容性。这些方法虽已标 `@deprecated`，但未通过 Level 递增声明 ABI 断裂。
- **消解方案**（推荐）：
  1. **当前 Level 内**（v3.x）：保留 public 可见性，仅追加 `[[deprecated("Use Manager::initialize() instead")]]` 编译期警告
  2. **下一 Level**（v4.x，未来版本）：将三个方法私有化，并递增 Level
  3. [04 任务 1.3](04-implementation-tasks.md) 优先级降为 P3，与 Level 递增同步执行
- **行动项**（✅ 已确认，已在 02 §2 / 04 任务 1.3 中落地修订）：
  - 修订 [02 §2](02-interface-design.md)：当前 Level 仅加 `[[deprecated]]`，私有化推迟到下一 Level ✅ 已修订
  - 修订 [04 任务 1.3](04-implementation-tasks.md)：调整优先级与执行条件 ✅ 已修订（P3，v3.x 仅警告，v4.x 私有化）

### ARCH-03：组合优于继承

- **要求**：不加深继承链；新增功能优先使用组合
- **契合度**：✅ 合规
- **证据**：
  - [README §5](README.md) 明确"不重构三层继承为组合"（避免过度设计、风险高）
  - 新增功能通过 `PackageManager::Impl` / `Manager::Impl` 内部组合实现，不新增继承层
  - [02 §3](02-interface-design.md) `contextKey` 通过 `ModuleSpec::Impl` 持有，组合方式注入
- **行动项**：无

### ARCH-04：相似模块统一设计

- **要求**：>60% 相同代码的模块提取公共实现，禁止复制粘贴创建新插件
- **契合度**：N/A（本方案不新增相似模块）
- **证据**：本方案为漏洞修复 + 接口加固，不引入新插件
- **行动项**：无（保留原则作为后续新增 Step 类型的约束）

### ARCH-05：结构化控制流

- **要求**：禁止应用层 goto（RAII cleanup 除外）；用 if-else / 提前 return / 子函数提取替代
- **契合度**：✅ 合规
- **证据**：
  - [02 §3.4](02-interface-design.md) ModelStep FQID 两级查找使用 `if-else` 链：先 `find(fqid)`，失败回退 `find(bareId)`
  - [02 §6](02-interface-design.md) `initialize()` 幂等防护为简单 if-guard：`if (m_initialized) return Error(...);`
- **行动项**：无

---

## 第二章：ROBUST — 健壮性与错误处理

### ROBUST-01：Expected\<T\> 传播错误

- **要求**：可能失败的函数返回 `Expected<T>`；调用方先检查状态再 `.take()`
- **契合度**：✅ 合规
- **证据**：
  - [02 §4](02-interface-design.md) `contextState()` / `failedContexts()` 返回值类型（非 Expected）— **设计理由**：纯查询操作不会失败，返回 `ContextState` 枚举（含 `NotRegistered`）表达所有可能状态
  - [02 §6](02-interface-design.md) `initialize()` 返回 `Expected<void>`，幂等检查失败返回 `Error{AlreadyInitialized}`
- **行动项**（✅ 已确认）：`ContextState` 枚举为四态 `Pending` / `Ready` / `Failed` / `NotRegistered`，已在 [02 §4.2](02-interface-design.md) 落地。`contextState()` 未命中返回 `NotRegistered`（非 `Pending`），与 SingerInfo.resolutionState 三态分层独立。

### ROBUST-02：异常边界隔离

- **要求**：try-catch 仅用于第三方库边界；每个 catch 必须记录日志或返回错误
- **契合度**：✅ 合规（无新增边界）
- **证据**：本方案所有新增 API 不涉及第三方库调用（无 JSON / ONNX / std::regex 新调用点）
- **行动项**：无

### ROBUST-03：catch 禁止静默吞掉异常

- **要求**：禁止空 catch 块；禁止 `catch(...)` 不记录不返回
- **契合度**：✅ 合规（无新增 catch）
- **证据**：本方案不新增 try-catch 块
- **行动项**：无

### ROBUST-04：空指针防御性检查

- **要求**：对 `spec` / `_impl` / 外部指针使用前检查 nullptr
- **契合度**：⚠️ 需注意
- **证据**：
  - [02 §3.4](02-interface-design.md) ModelStep 修复后保留 `if (!g2pCate) nullptr` 检查（[ModelStep.cpp:45-49](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp)）
  - [02 §3.1](02-interface-design.md) `ModuleSpec::contextKey()` 新增 getter 需检查 `_impl` 是否非空
- **行动项**：实施 [04 任务 1.1](04-implementation-tasks.md) 时，`contextKey()` 实现必须为：
  ```cpp
  ContextKey ModuleSpec::contextKey() const {
      return _impl ? _impl->contextKey : ContextKey{};
  }
  ```
  并在测试中覆盖 `_impl == nullptr` 路径

### ROBUST-05：容器元素迭代器失效防护

- **要求**：`remove_if` / `erase` 后不使用旧迭代器
- **契合度**：✅ 合规
- **证据**：
  - `failedContexts()` 实现为：遍历 `contextStates` 构建 `std::vector<ContextKey>` 返回，不修改原容器
  - 本方案不引入新的 `remove_if` / `erase` 路径
- **行动项**：无

---

## 第三章：INFRA — 基础设施与配置

### INFRA-01：ConfigAccessor Required/Optional 分离

- **要求**：必须字段用 `cfg.getString("key")`，可选字段用 `cfg.getString("key", "default")`
- **契合度**：N/A（本方案不新增配置访问）
- **证据**：`contextKey` 通过构造期注入（`createModuleTask` 阶段从 `ModuleMetadata.context` 写入 `Impl::contextKey`），非配置读取
- **行动项**：无

### INFRA-02：日志分类约定

- **要求**：每个模块声明独立 `LogCategory`；使用 `%1`/`%2` 占位符
- **契合度**：✅ 合规
- **证据**：
  - ModelStep 已有 `static LangCore::LogCategory ModelLog("chainG2p.model");`
  - `failedContexts()` 实现不需新增 LogCategory（仅查询，无新日志输出）
  - 若实施 [04 任务 1.11](04-implementation-tasks.md) `initialize()` 幂等防护，应在已有 Manager 日志分类下输出 `langCoreWarning("initialize() called twice, ignored")`
- **行动项**：无

### INFRA-03：插件注册规范

- **要求**：使用 `LANGCORE_DEFINE_TASK_PLUGIN` / `LANGCORE_DEFINE_DRIVER_PLUGIN` 宏
- **契合度**：N/A（本方案不新增插件）
- **证据**：ModelStep 是 ChainG2p 内部 Step，非独立插件，无需注册
- **行动项**：无

### INFRA-04：CMake 项目命名约定

- **要求**：`project()` 名称与注册 key 一致；使用 `LangPlugins_add_plugin()`
- **契合度**：N/A（本方案不新增 CMake 目标）
- **行动项**：无

### INFRA-05：测试分级管理

- **要求**：L1（纯逻辑）/ L2（需插件 DLL）/ L3（端到端）；共享代码入 `tests/common/`
- **契合度**：⚠️ 需注意
- **证据**：
  - [04 任务 1.5](04-implementation-tasks.md) S5 场景端到端测试 → **L3**，应入 `tests/tst_langCore/`
  - [04 任务 1.4](04-implementation-tasks.md) `failedContexts()` 单元测试 → **L2**（需 PackageManager + 模块加载），应入 `tests/catch2/`
- **行动项**：实施时按以下规则放置新测试：
  - `tests/catch2/TestPackageManagerContext.cpp`（L2 单元）
  - `tests/tst_langCore/` 下新增 S5 场景配置（L3 集成）

---

## 第四章：PACK — 包管理准则

### PACK-01：依赖显式声明与静态校验

- **要求**：模块间依赖在 `package.json` 的 `dependencies` 字段显式声明；`DependencyResolver` 静态校验
- **契合度**：✅ 合规（不修改依赖声明机制）
- **证据**：
  - ModelStep 引用 LstmG2p 任务的方式是 **运行期 task 查找**（`category("g2p")->getFirstObject(id)`），非 package.json 静态依赖
  - 设计意图：ChainG2p 通过 config.json 的 step 配置声明引用 id，Framework 负责按 context 路由查找
  - 本方案不改变此机制，仅修正查找路径（FQID 两级查找）
- **行动项**：无

### PACK-02：依赖解析防循环

- **要求**：`DependencyGraph::findCycles()` 在加载前检测环
- **契合度**：✅ 合规
- **证据**：本方案不修改依赖图构建逻辑；`failedContexts()` 仅查询 `contextStates`，不参与图操作
- **行动项**：无

---

## 第五章：核对结果汇总

### 5.1 合规统计

| 章节 | 总数 | ✅ 合规 | ⚠️ 需注意 | ❌ 违反 | N/A |
|------|------|---------|-----------|---------|-----|
| ARCH | 5 | 4 | 1 (ARCH-02) | 0 | 1 (ARCH-04) |
| ROBUST | 5 | 4 | 1 (ROBUST-04) | 0 | 0 |
| INFRA | 5 | 2 | 1 (INFRA-05) | 0 | 3 (INFRA-01/03/04) |
| PACK | 2 | 2 | 0 | 0 | 0 |
| **合计** | **17** | **12** | **3** | **0** | **4** |

**结论**：本方案无违反项；3 项需注意项均已在"行动项"中给出消解措施。

### 5.2 行动项清单（实施前修订）

| # | 关联原则 | 修订对象 | 修订内容 | 状态 |
|---|---------|---------|---------|------|
| A-1 | ARCH-02 | [02 §2](02-interface-design.md) | 当前 Level 仅追加 `[[deprecated]]` 警告，私有化推迟到下一 Level（v4.x） | ✅ 已确认（已落地修订） |
| A-2 | ARCH-02 | [04 任务 1.3](04-implementation-tasks.md) | 优先级降为 P3，绑定下一 Level 递增 | ✅ 已确认（已落地修订） |
| A-3 | ROBUST-04 | [04 任务 1.1](04-implementation-tasks.md) | `ModuleSpec::contextKey()` 实现需 nullptr 防御，测试覆盖空 `_impl` 路径 | ⬜ 待实施 |
| A-4 | INFRA-05 | [04 任务 1.4 / 1.5](04-implementation-tasks.md) | 测试分级：1.4 入 `tests/catch2/` (L2)，1.5 入 `tests/tst_langCore/` (L3) | ⬜ 待实施 |

### 5.3 不冲突原则（实施时遵循）

以下原则虽无冲突，但在实施过程中需持续遵循：

- **ARCH-01**：ModelStep 修复不得引入内部重试逻辑
- **ARCH-05**：FQID 两级查找保持 if-else 结构，不引入 goto
- **ROBUST-01**：`failedContexts()` 返回值类型为 `std::vector<ContextKey>`（非 Expected），因查询不失败
- **ROBUST-02/03**：本方案无新 try-catch；后续若 `initialize()` 幂等防护涉及第三方调用，按边界隔离原则处理
- **INFRA-02**：`initialize()` 二次调用日志使用现有 Manager 日志分类，不新增 LogCategory
- **PACK-01**：不修改 package.json 依赖声明；ModelStep 引用 LstmG2p 仍为运行期 task 查找

---

## 第六章：与 human-decisions 附录的关系

[human-decisions.md](../decisions/human-decisions.md) 附录 A（已废止决策）当前为空，本方案不新增废止项。

[human-decisions.md](../decisions/human-decisions.md) 附录 B（ADR 冲突解决记录）暂无与本方案相关的冲突条目。若实施过程中出现新的冲突决策（如 `failedContexts()` 返回值类型争议），应补充到附录 B。

---

**文档版本**: 1.0
**关联文档**: [README.md](README.md) · [01-current-state-audit.md](01-current-state-audit.md) · [02-interface-design.md](02-interface-design.md) · [03-host-integration-contract.md](03-host-integration-contract.md) · [04-implementation-tasks.md](04-implementation-tasks.md) · [decisions/human-decisions.md](../decisions/human-decisions.md)
