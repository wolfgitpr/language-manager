# Language Manager 重构方案（保守版）

> 2026-05-19 — 参考 [dataset-tools 项目的优秀设计实践](../../D:/projects/dataset-tools/docs/)，对照本项目的架构现状和真实痛点，提出保守重构方案。
>
> **保守原则**：仅解决项目实际存在的问题；不引入不必要的抽象层；每一项优化必须有明确的收益和可验证的成果。

---

## 一、参考来源与对照分析

### 1.1 dataset-tools 的优秀设计实践

本方案的设计方法论主要参考 dataset-tools 项目的以下文档：

| 文档 | 核心价值 | 本项目对应状况 |
|------|---------|-------------|
| [human-decisions.md](file:///D:/projects/dataset-tools/docs/human-decisions.md) | 统一的设计准则目录（ARCH/CONCUR/ROBUST/INFRA/VIEW 分类，含唯一编号、理由、禁止模式、正确做法、关联关系） | ❌ 缺失。本项目的设计准则分散在 [PRD-v2.0.md §1.1](file:///d:/projects/language-manager/docs/PRD-v2.0.md#L17-L23) 和 [Conventions-and-Standards.md §9](file:///d:/projects/language-manager/docs/Conventions-and-Standards.md#L289-L297) 中，无统一编号和交叉引用 |
| [framework-architecture.md](file:///D:/projects/dataset-tools/docs/design/framework-architecture.md) | 分层架构 + 模块依赖图 + 命名规范矩阵 + 框架接口一览 + 已知架构问题 | ✅ 已有 [Architecture-Overview.md](file:///d:/projects/language-manager/docs/Architecture-Overview.md)，质量良好 |
| [comprehensive-analysis.md](file:///D:/projects/dataset-tools/docs/analysis/comprehensive-analysis.md) | 全量源码静态分析：重复率、技术债、架构符合度评分 | ❌ 缺失。仅有 [Issues-Tracker.md](file:///d:/projects/language-manager/docs/Issues-Tracker.md) 基于手动代码审计（PRD §14），非系统性分析 |
| [architecture-optimization.md](file:///D:/projects/dataset-tools/docs/analysis/architecture-optimization.md) | 保守优化项 + 明确不做什么 + 验证标准 | ❌ 缺失。本项目无同类文档 |
| [test-design.md](file:///D:/projects/dataset-tools/docs/design/test-design.md) | 分层测试 + 用例矩阵 + Mock/Fixture 规范 + Review 清单 + 实施优先级 | ⚠️ [Test-Design-Document.md](file:///d:/projects/language-manager/docs/Test-Design-Document.md) 大部分是规划描述（13+ 测试目录），实际仅 3 个目录 |
| [conventions.md](file:///D:/projects/dataset-tools/docs/guides/conventions.md) | 完整的编码/错误处理/异步/接口/CMake/Git 规范 | ✅ 已有 [Conventions-and-Standards.md](file:///d:/projects/language-manager/docs/Conventions-and-Standards.md)，覆盖面完整 |
| [pipeline.md](file:///D:/projects/dataset-tools/docs/design/pipeline.md) | 流水线 I/O 契约 + ADR 记录 + 模型可替换性 | ✅ 已有 [ChainG2p-Design-Document.md](file:///d:/projects/language-manager/docs/ChainG2p-Design-Document.md)，流水线设计完善 |
| [dirty-mechanism.md](file:///D:/projects/dataset-tools/docs/design/dirty-mechanism.md) | 层依赖 DAG + 脏数据传播 + 步骤级标脏 + UI 表现 | ⚠️ 本项目无等效概念。G2p 框架无编辑场景，不适用此机制 |

### 1.2 本项目已有的优势（不比 dataset-tools 差的方面）

| 方面 | 本项目的优势 |
|------|------------|
| 插件架构 | Level/Version 双重校验体系优于 dataset-tools 的单层接口隔离 |
| 错误处理 | `Expected<T>` + `Error` 的 11 种错误码 + `G2pErrorType` 双层模型，比 dataset-tools 的 `Result<T>` 更细粒度 |
| 配置管理 | `ConfigAccessor` 的 Required/Optional 分离 + `ValidationChain` 链式验证 |
| 多版本支持 | `VersionedTaskManager` + `TASK_IMPLEMENT` 宏体系，dataset-tools 无等效机制 |
| 包系统 | Package/ModuleCategory/DependencyGraph 完整闭环 |
| 设计评审 | PRD §14 的 25+ 项代码审计和修复记录，比 dataset-tools 做得更彻底 |

---

## 二、核心优化项

### REF-OP-01：消除 MandarinG2p / CantoneseG2p 的 70%+ 代码重复

**问题严重性**：**P1（中等）** — 两个插件 187 行 vs 186 行的实现文件有 ~140 行完全相同。这不仅增加维护负担，更埋下 bug 不对称的隐患（已有先例：PRD §14.11 的 mode 分类、§14.12 的 getConfig 缓存都曾只在其中一个插件中缺失）。

**当前状态**（经代码审计核实）：

```cpp
// 两个文件的结构完全一致：
//
// MandarinG2p/internal/V1/TaskImpl.cpp  (187 行)
// CantoneseG2p/internal/V1/TaskImpl.cpp (186 行)
//
// 相同部分：
//   - initialize():         读取 dictPath、验证字段、缓存 m_config
//   - start():              groupLyrics() 函数体完全相同（按 g2pId/mode 分组）
//                           hanziToPinyin() 调用 + try-catch + 结果构造逻辑完全相同
//   - getConfig():          构造 JSON 的逻辑完全相同
//   - 成员变量:              m_dictPath, m_configName, m_config 声明相同
//
// 唯一差异：
//   - 底层库调用:            cpp-pinyin 的初始化参数（Mandarin vs Cantonese 标志）
//   - 默认 dict 路径:        "ds-zh-pinyin-lite.txt" vs "ds-yue-jyutping.txt"
//   - include:              <cpp-pinyin/Mandarin.h> vs <cpp-pinyin/Cantonese.h>
```

**修复方案**：提取公共基类 `PinyinG2pTaskImplBase`，通过模板参数或构造函数参数注入差异点。

```cpp
// 新文件: plugins/G2ps/Common/PinyinG2pTaskImplBase.h
namespace LangPlugins::Common {

class PinyinG2pTaskImplBase : public LangCore::VersionedTaskImplBase {
public:
    struct Config {
        std::string dictPathKey;          // "dictPath" / "jyutpingDictPath"
        std::string defaultDictFileName;  // "ds-zh-pinyin-lite.txt" / "ds-yue-jyutping.txt"
        std::string languageName;         // "Mandarin" / "Cantonese" (for logging)
    };

    explicit PinyinG2pTaskImplBase(const LangCore::ModuleSpec *spec, Config config);

    LangCore::Expected<void> initialize() override;
    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;
    std::string getConfig() const override;

private:
    Config m_langConfig;
    std::string m_dictPath;
    std::string m_configName;
    std::string m_config;
};

} // namespace LangPlugins::Common
```

子类变为一行委托：

```cpp
// MandarinG2p/internal/V1/TaskImpl.h
class MandarinG2pTaskImpl : public LangPlugins::Common::PinyinG2pTaskImplBase {
public:
    explicit MandarinG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : PinyinG2pTaskImplBase(spec, {
              "dictPath",
              "ds-zh-pinyin-lite.txt",
              "Mandarin"
          }) {}
};
```

`start()` 中的 `hanziToPinyin()` 调用通过虚方法或 lambda 注入，子类提供差异化的拼音转换逻辑。

**涉及文件**：

| 操作 | 文件 |
|------|------|
| 新增 | `plugins/G2ps/Common/PinyinG2pTaskImplBase.h` |
| 新增 | `plugins/G2ps/Common/PinyinG2pTaskImplBase.cpp` (~160 行，提取公共逻辑) |
| 修改 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.h` (简化为 ~10 行) |
| 修改 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.cpp` (简化为 ~20 行) |
| 修改 | `plugins/G2ps/CantoneseG2p/internal/V1/TaskImpl.h` (简化为 ~10 行) |
| 修改 | `plugins/G2ps/CantoneseG2p/internal/V1/TaskImpl.cpp` (简化为 ~20 行) |
| 修改 | `plugins/G2ps/MandarinG2p/CMakeLists.txt` (添加 Common 依赖) |
| 修改 | `plugins/G2ps/CantoneseG2p/CMakeLists.txt` (添加 Common 依赖) |

**收益**：
- 消除 ~140 行重复代码
- 修复一个插件时不会忘记另一个（杜绝不对称 bug）
- 新增同类拼音 G2p 插件（如日后支持吴语、闽南语）时只需 30 行以内的新代码

**风险**：低。两个类结构几乎相同，提取的边界清晰。`hanziToPinyin()` 的差异通过模板或 lambda 注入即可处理。

**参考**：dataset-tools 的 [ARCH-04](file:///D:/projects/dataset-tools/docs/human-decisions.md#arch-04相似模块统一设计)「相似模块统一设计」原则——"两个类有 >60% 相同代码 → 合并为同一类 + 配置开关"。

---

### REF-OP-02：统一测试框架头文件（消除 tst_framework.h 重复）

**问题严重性**：**P2（低）** — 同一个 `tst_framework.h` 文件在两个目录中各自维护了一份完整副本：

```
tests/tst_unit/tst_framework.h      # 复制 A
tests/tst_context/tst_framework.h   # 复制 B
```

两个文件提供相同的 `TEST_CASE`、`ASSERT_*` 宏和注册表逻辑。任何对测试框架的改进都需要在两个位置分别修改。

**修复方案**：创建 `tests/common/tst_framework.h`，两个测试目录改为 `#include "../common/tst_framework.h"`。

**涉及文件**：

| 操作 | 文件 |
|------|------|
| 新增 | `tests/common/tst_framework.h`（从现有两份中任选一份） |
| 删除 | `tests/tst_unit/tst_framework.h` |
| 删除 | `tests/tst_context/tst_framework.h` |
| 修改 | `tests/tst_unit/CMakeLists.txt`（添加 common 的 include 路径） |
| 修改 | `tests/tst_context/CMakeLists.txt`（添加 common 的 include 路径） |

**收益**：消除重复的测试基础设施文件，降低未来修改测试框架的成本。

**风险**：极低。纯文件移动，不改变任何逻辑。

**参考**：dataset-tools 的 [ARCH-OP-04](file:///D:/projects/dataset-tools/docs/analysis/architecture-optimization.md#arch-op-04测试基础设施最小化建设)「测试基础设施最小化建设」——提取共享的测试基类和 Mock 库。

---

### REF-OP-03：Session::close 控制流重构（goto → 结构化）

**问题严重性**：**P1（中等）** — [PRD §14.17](file:///d:/projects/language-manager/docs/PRD-v2.0.md#L859-L864) 已识别，[Issues-Tracker §1.3](file:///d:/projects/language-manager/docs/Issues-Tracker.md#L101-L120) 已记录。

**问题**：`OnnxDriver/internal/Session.cpp` 的 `open()` 和 `close()` 使用 4 个 `goto` 标签（`out_exists`、`out_search_hash`、`out_success`），正确的运行依赖对 `path_map` / `hash_size_map` / `image_list` 三重索引的心智模型。后续修改极易引入回归 bug。

**修复方案**：提取子函数替代 goto 标签：

```
open() 的 goto 标签           → 提取为子函数
─────────────────────────────────────────────────────
out_exists (line 505, 531)    → tryReuseExistingSession()
out_search_hash (line 511, 535) → createSessionFromPathHash()
out_success (line 564, 569)   → 正常返回，无需标签

close() 的 goto 标签
─────────────────────
out_success (line 597, 614)   → releaseSessionResources()
```

**涉及文件**：`plugins/Drivers/OnnxDriver/internal/Session.cpp` (~150 行受影响范围)

**收益**：
- 消除 4 个 goto 标签，控制流可静态分析
- 新函数可独立进行单元测试
- 遵循 [ARCH-01](file:///D:/projects/dataset-tools/docs/human-decisions.md#arch-01模块职责单一行为不得分散重复)「模块职责单一」——每个子函数管理 Session 的不同阶段

**风险**：中。Session 是并发热路径，重构需保证线程安全语义不变。建议添加 `SessionLifecycle` 单元测试后再重构。

---

### REF-OP-04：填补核心 L1 测试缺口（tst_dependency / tst_support）

**问题严重性**：**P1（中等）** — [Test-Design-Document.md](file:///d:/projects/language-manager/docs/Test-Design-Document.md) 详细规划了 13+ 测试目录，但当前仅实现了 3 个：

| 规划 | 实际状态 |
|------|---------|
| `tst_support` (Error, Expected, ConfigAccessor, ValidationChain) | ❌ 未实现。部分用例零散分布于 `tst_unit/`（5 个文件），但用例覆盖不足 |
| `tst_version` (VersionRange, VersionResolver) | ⚠️ 部分覆盖：`tst_unit/tst_version_dep.cpp` 同时覆盖了版本和依赖，混合粒度 |
| `tst_dependency` (DependencyResolver, DependencyGraph, LevelChecker) | ⚠️ 同上 |
| `tst_package` (Package 解析, PackageManager) | ❌ 未实现 |
| `tst_plugin` (插件加载, Task 生命周期) | ❌ 未实现 |
| `tst_dict` (DsDict) | ❌ 未实现 |
| `tst_integration` (端到端 G2p 管线) | ✅ 已实现 (`tst_langCore/`) |
| `tst_context` (Context 隔离、FQID、去重) | ✅ 已实现 |

**核心问题**：依赖解析子系统（DependencyResolver + DependencyGraph + LevelCompatibilityChecker）是框架最复杂的部分，拥有 [Test-Design-Document.md §5](file:///d:/projects/language-manager/docs/Test-Design-Document.md#L308-L320) 中规划的 10+ 种依赖拓扑场景（跨包、菱形、链式、循环、多版本），但实际测试中这些场景混合在 `tst_version_dep.cpp` 一个文件中，且没有 fixture 目录结构。

**修复方案**（最小化路径——不追求 13 个目录全覆盖）：

**第一批（P0 — 当前最需要）**：

1. **`tst_dependency/`** — 覆盖依赖解析核心场景
   - `tst_dependency_resolver.cpp`：跨包依赖、菱形依赖、循环检测、版本冲突
   - `tst_dependency_graph.cpp`：拓扑排序、环检测、包级排序
   - `fixtures/packages/`：mock package 目录结构（参考 [Test-Design-Document.md §5.1](file:///d:/projects/language-manager/docs/Test-Design-Document.md#L315-L388)）

2. **`tst_support/`** — 补充 Error/Expected 的缺失用例
   - `tst_error.cpp`：全部 11 种错误码、withContext/withExtra 链式调用、fullMessage 格式
   - `tst_expected.cpp`：移动语义、Expected\<void\>、valueOr、SFINAE 约束

**第二批（P1 — 按需追加）**：

3. **`tst_package/`** — Package 解析的异常路径
4. **`tst_plugin/`** — 插件加载 + Task 生命周期（需要 `LANGMGR_BUILD_PLUGINS=ON`）

**涉及文件**（第一批）：

| 操作 | 文件 |
|------|------|
| 新增 | `tests/tst_dependency/CMakeLists.txt` |
| 新增 | `tests/tst_dependency/tst_dependency_resolver.cpp` |
| 新增 | `tests/tst_dependency/tst_dependency_graph.cpp` |
| 新增 | `tests/tst_dependency/fixtures/packages/scenario-cross-dep/...` |
| 新增 | `tests/tst_dependency/fixtures/packages/scenario-diamond/...` |
| 新增 | `tests/tst_dependency/fixtures/packages/scenario-cycle-cross/...` |
| 新增 | `tests/tst_support/CMakeLists.txt` |
| 新增 | `tests/tst_support/tst_error.cpp` |
| 新增 | `tests/tst_support/tst_expected.cpp` |
| 修改 | `tests/CMakeLists.txt`（注册新子目录） |

**收益**：
- 依赖解析子系统有独立、可复现的回归测试，不再依赖集成测试发现 bug
- 新增 mock package fixtures 可被后续 `tst_package` 和 `tst_plugin` 复用
- 遵循 [ROBUST-03](file:///D:/projects/language-manager/docs/PRD-v2.0.md#L17-L23)「简洁可靠」——有测试才能放心重构

**风险**：低。这些测试均为 L1 级别，不需要插件 DLL 或外部库，纯逻辑验证。

**参考**：dataset-tools 的 [test-design.md](file:///D:/projects/dataset-tools/docs/design/test-design.md) 中的分层测试架构和 fixture 组织方式。

---

### REF-OP-05：创建设计准则统一目录（human-decisions）

**问题严重性**：**P2（低）** — 本项目的设计准则散落在三处：

| 来源 | 内容 |
|------|------|
| [PRD-v2.0.md §1.1](file:///d:/projects/language-manager/docs/PRD-v2.0.md#L17-L23) | 5 条设计原则（简洁可靠、接口稳定、长期免维护、可接受不兼容、异常边界隔离） |
| [Conventions-and-Standards.md §9](file:///d:/projects/language-manager/docs/Conventions-and-Standards.md#L289-L297) | 重复了同样的 5 条原则 |
| [PRD-v2.0.md §14](file:///d:/projects/language-manager/docs/PRD-v2.0.md#L769-L916) | 25+ 项设计评审记录（蕴含了大量隐含原则，但未显式提取为准则） |

当前缺失的：
- 原则间没有编号和交叉引用（比如"异常边界隔离"和 `Expected<T>` 错误处理规范、"接口稳定"和 Level/Version 体系之间的关系在文档中没有显式链接）
- 没有"禁止模式"的正反例对照
- PRD §14 的设计评审发现（如"catch 禁止静默吞掉"、"task Mgr() 必须检查 nullptr"）应该提炼为通用准则

**修复方案**：新增 `docs/decisions/human-decisions.md`，参照 dataset-tools 的分类体系：

```
# 设计准则与决策

## 第一章：ARCH — 架构与模块设计
    ARCH-01：插件职责单一（来源：PRD §1.1 "简洁可靠"）
    ARCH-02：接口稳定，Level 锚定兼容性（来源：PRD §2.1）
    ARCH-03：组合优于继承（来源：未显式声明，但 VersionedTaskManager 即是实践）
    ARCH-04：相似模块统一设计（来源：新提炼，对应 REF-OP-01）
    ...

## 第二章：ROBUST — 健壮性与错误处理
    ROBUST-01：Expected<T> 传播错误（来源：Conventions §3）
    ROBUST-02：异常边界隔离（来源：PRD §5.4）
    ROBUST-03：catch 禁止静默吞掉（来源：PRD §14.6）
    ...

## 第三章：INFRA — 基础设施与配置
    INFRA-01：ConfigAccessor Required/Optional 分离
    INFRA-02：日志分类约定
    ...

## 附录A：已废止决策
## 附录B：ADR 冲突解决记录
```

**不做的**：不创建 view 类原则（本项目无 UI 层），不创建 concurrency 类原则（当前无异步需求）。

**涉及文件**：

| 操作 | 文件 |
|------|------|
| 新增 | `docs/decisions/human-decisions.md` |
| 修改 | `docs/Index.md`（添加新文档链接） |

**收益**：
- 所有设计决策有唯一编号，后续文档和代码 review 可以精确引用（"这违反了 ROBUST-03" 比 "catch 要记日志" 更可追溯）
- 新加入的开发者可以快速理解项目的设计理念全貌
- PRD §14 发现的 bug 修复经验被固化为准则，防止同类问题重现

**风险**：极低。纯文档整理，不涉及代码变更。

**参考**：dataset-tools 的 [human-decisions.md](file:///D:/projects/dataset-tools/docs/human-decisions.md) 是本方案的核心参考来源——其分层编号体系和交叉引用设计是本项目最应该吸收的文档实践。

---

### REF-OP-06：测试目标合并（多个 → 单一 Catch2 目标）

**问题严重性**：**P2（低）** — 测试分为 4 个独立 CMake 目标（`tst_unit`/`tst_context`/`tst_dependency`/`tst_support`），在 CLion 中显示为 4 项，不利于整体运行。每个目录各有独立 `main.cpp`（都定义了 `CATCH_CONFIG_MAIN`），浪费编译时间。

**修复方案**：参考 [cpp-pinyin 的测试结构](file:///D:/projects/cpp-pinyin/tests/CMakeLists.txt)——将所有 Catch2 测试源文件通过 `file(GLOB)` 收集为单一可执行目标 `LangMgrTests`，仅一个 `main.cpp` 定义 `CATCH_CONFIG_MAIN`。

```
变更前：                               变更后：
tests/tst_unit → tst_unit.exe         tests/catch2/ → LangMgrTests.exe
tests/tst_context → tst_context.exe     (单一目标，CLion 显示一项)
tests/tst_dependency → tst_dependency.exe
tests/tst_support → tst_support.exe   tests/tst_langCore/ 保留为最小例程
```

**涉及文件**：

| 操作 | 文件 |
|------|------|
| 新增 | `tests/catch2/CMakeLists.txt` |
| 新增 | `tests/catch2/main.cpp`（唯一 CATCH_CONFIG_MAIN） |
| 移动 | `tests/{tst_unit,tst_support,tst_dependency,tst_context}/*.cpp → tests/catch2/` |
| 修改 | `tests/CMakeLists.txt`（简化为 add_subdirectory(catch2) + tst_langCore） |
| 修改 | `tests/tst_langCore/CMakeLists.txt`（补充 add_test） |
| 删除 | `tests/tst_unit/` `tests/tst_context/` `tests/tst_dependency/` `tests/tst_support/` |

**收益**：
- CLion CMake 面板中测试从 4 项缩减为 1 项
- 单一 `CATCH_CONFIG_MAIN` 减少编译时间
- 与 cpp-pinyin 的 `CppPinyinTests` 模式一致

**风险**：极低。纯文件移动 + CMake 重构，不改变测试逻辑。

**参考**：[cpp-pinyin tests/CMakeLists.txt](file:///D:/projects/cpp-pinyin/tests/CMakeLists.txt)

---

## 三、明确不做的事情

以下优化方案经评估后**不予执行**，原因列明：

| 方案 | 不做的原因 |
|------|----------|
| 继承链改组合（PRD §14.7） | 三层 public 继承虽然不够优雅，但当前工作正常。改动涉及 `Manager` → `PackageManager` → `PluginFactory` 全链路 + stdcorelib pimpl 约定，风险高且收益不可量化（接口暴露范围过大虽然是问题，但目前没有因此产生 bug） |
| 测试框架从自定义迁移到 Qt Test | [Test-Design-Document.md](file:///d:/projects/language-manager/docs/Test-Design-Document.md) 规划的 Qt Test 迁移工作量大（需重写所有现有测试用例、学习 QTest 数据驱动语法、处理 QApplication 依赖），而当前自定义框架足够轻量且满足需求。仅当出现当前框架无法支持的测试需求时才考虑迁移 |
| 大文件拆分 | 本项目最大文件不足 500 行（LstmG2p V2 TaskImpl.cpp ~400 行，Session.cpp ~620 行含 goto 重构目标），远低于 dataset-tools 的 1000+ 行阈值。不存在大文件拆分需求 |
| 创建 apps → plugins 分层封堵（如 dataset-tools 的 ARCH-OP-01） | 本项目没有 apps 层，插件间的依赖通过 `package.json` 的 `dependencies` 显式声明、`DependencyResolver` 静态校验、CMake `target_link_libraries` 编译期 enforce，已经比 dataset-tools 的 libs 封装方案更严格。不存在分层违规问题 |
| 引入 dirty/标脏机制 | G2p 框架是纯转换管线（输入文本 → 输出音素），没有编辑/修改场景，不存在"数据被修改后下游需感知"的需求。dirty 机制不适用 |
| PackageManager::open 传递依赖加载 | Issues-Tracker §2.1 记录的功能缺失已明确：当前主流程使用 `loadPackagesInOrder()` 按拓扑序加载，不受影响。实现此功能的前置需求（运行时动态加载单个包）不存在 |
| 静态插件链接选项 | Issues-Tracker §2.2 记录的未消费 CMake 选项。移除该选项即可（一行代码），无需实现完整静态链接功能 |
| 统一 QTEST_MAIN / QTEST_GUILESS_MAIN | 本项目不使用 Qt Test，无此问题 |

---

## 四、执行优先级汇总

| 优先级 | 编号 | 项目 | 范围 | 预估工时 | 风险 |
|--------|------|------|------|---------|------|
| **P0** | REF-OP-01 | 消除 MandarinG2p/CantoneseG2p 重复 | 8 文件（2 新增 + 6 修改） | 3-4h | 低 |
| **P1** | REF-OP-03 | Session::close goto 重构 | 1 文件 | 2-3h | 中 |
| **P1** | REF-OP-04 | 填补核心 L1 测试缺口 | ~8 文件 | 4-6h | 低 |
| **P2** | REF-OP-02 | 统一测试框架头文件 | 4 文件 | 30min | 极低 |
| **P2** | REF-OP-05 | 创建设计准则统一目录 | 2 文件（纯文档） | 2-3h | 极低 |

---

## 五、验证标准

| 优化项 | 验证方式 |
|--------|---------|
| REF-OP-01 | `ctest --test-dir build -C Debug` 全部通过；MandarinG2p 和 CantoneseG2p 的 G2p 转换结果与重构前完全一致 |
| REF-OP-02 | `cmake --build build --config Debug` 成功；不存在 `tst_unit/tst_framework.h` 和 `tst_context/tst_framework.h` 两个独立副本 |
| REF-OP-03 | 所有现有 ONNX 推理测试通过；Session::open/close 行为与重构前一致 |
| REF-OP-04 | `ctest --test-dir build -C Debug` 新增至少 20 个测试用例；覆盖率上升 |
| REF-OP-05 | 新文档包含所有现有原则 + PRD §14 的隐含原则；与现有文档不冲突 |

---

## 六、与现有文档的关系

本文档是对 [Issues-Tracker.md](file:///d:/projects/language-manager/docs/Issues-Tracker.md) 和 [PRD-v2.0.md §14](file:///d:/projects/language-manager/docs/PRD-v2.0.md#L769-L916) 的补充：

| 文档 | 关注点 |
|------|--------|
| Issues-Tracker.md | 单个 bug/功能缺失的记录和追踪 |
| PRD §14 | 设计评审发现的已修复/待修复问题 |
| **本文档** | 系统性优化：代码重复消除、测试覆盖补充、文档体系完善 |

**注意**：本文档不重复 Issues-Tracker 中已记录的事项（如 FormatStep 命名、V2 EOS 性能），除非需要从架构层面解决（如 REF-OP-03 是 Issues-Tracker §1.3 的落实方案）。

---

**文档版本**: 1.0  
**关联文档**: [PRD-v2.0.md](file:///d:/projects/language-manager/docs/PRD-v2.0.md) · [Issues-Tracker.md](file:///d:/projects/language-manager/docs/Issues-Tracker.md) · [Test-Design-Document.md](file:///d:/projects/language-manager/docs/Test-Design-Document.md) · [dataset-tools 架构优化方案](file:///D:/projects/dataset-tools/docs/analysis/architecture-optimization.md)