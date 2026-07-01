# Voice Bank Scoped Package 设计文档

**版本**：3.1  
**日期**：2026-06-30  
**关联 PRD**：PRD-v2.0.md §14  
**状态**：v3.1 确认版 — 保留 `ContextKey{context, version}` 双维度。v4.0 简化方案（移除 version 维度）已撤销。

> ⚠️ **v4.0 撤销声明**：本文档早期版本（v4.0）曾提议移除 `ContextKey` 的 `version` 维度，理由为"框架不需要感知版本"。经与调用方 ds-editor-lite 实施代码双向核对，该前提与实际数据流不符——version 维度被实际使用（voicebank context 回退到 `singer.packageVersion()`，参与 `addPackagePath`/`G2pInput`/`task` 全链路）。v4.0 方案予以撤销，恢复 v3.0 双维度设计。详见 [host-integration/02-context-isolation-mechanism.md](../host-integration/02-context-isolation-mechanism.md)。

---

## 1. 设计原则

系统设计上只允许声库**指定使用官方默认 G2P**、或者**携带自定义 G2P 的 Package**。不做多余抽象。

| 场景 | 行为 |
|------|------|
| 声库使用官方默认 G2P | 宿主不注册自定义包，`convert()` 时传 `context=""`, `version={}` |
| 声库携带自定义 G2P Package | 宿主调用 `addPackagePath("SingerA", version, path)`，`convert()` 时传 `context="SingerA"`, `version=singer.packageVersion()` |

**核心设计**：保留 `ContextKey{context, version}` 双维度。`context` 标识声库隔离域，`version` 标识声库包版本（voicebank context 未显式声明时回退到 `singer.packageVersion()`）。version 维度经调用方 ds-editor-lite 实施 + 12 个测试用例验证为必需。

---

## 2. 核心概念：Context

**Context**（上下文）= 声库名，作为 package 的归属标识。

- 每个 context 是一个独立的 package 隔离域
- 同一 context 内的模块按现有规则互相可见、可依赖
- 不同 context 之间的模块**互相不可见**
- **默认 context**（空字符串 `""`）：用于官方包，其模块对所有 context **全局可见**

### 2.1 完全限定 ID（FQID）

```
context 非空: FQID = context + ":" + moduleId
context 为空: FQID = moduleId
```

| 声库 | moduleId | FQID |
|------|----------|------|
| 官方 | `g2p-cmn-official` | `g2p-cmn-official` |
| SingerA | `g2p-cmn-custom` | `SingerA:g2p-cmn-custom` |
| SingerB | `g2p-cmn-custom` | `SingerB:g2p-cmn-custom` |

### 2.2 Context 内去重规则

同一 context 内，若多次加载产生了相同 `(moduleId, iid, level, version)` 的模块：

| 条件 | 处理 |
|------|------|
| 四元组 `(moduleId, iid, level, version)` 完全相同 | **共用**：只保留首次加载的，日志 `Information` |
| `moduleId` 相同但四元组不完全相同 | **共存**：都加载，`selectBestModules` 照常工作 |

### 2.3 默认 Context 的特殊性

1. **全局可见**：其他 context 的模块声明依赖时，先在本 context 内查找，找不到回退到默认 context
2. **反向不可见**：默认 context 的模块不能依赖其他 context 的模块
3. **初始化优先**：默认 context 必须先于所有其他 context 完成初始化

---

## 3. API 设计

### 3.1 PackageManager

```cpp
class PackageManager {
public:
    /// 在指定 context 下添加包搜索路径。
    Expected<void> addPackagePath(const std::string &context,
                                  const std::filesystem::path &path);

    /// 在指定 context 下批量设置包搜索路径。
    Expected<void> setPackagePaths(const std::string &context,
                                   const std::vector<std::filesystem::path> &paths);

    /// 获取指定 context 的包搜索路径。
    std::vector<std::filesystem::path> packagePaths(const std::string &context) const;

    /// 获取所有已注册的 context 名（含默认 context ""）。
    std::vector<std::string> contexts() const;
};
```

### 3.2 Manager

```cpp
class Manager : public PackageManager {
public:
    Expected<void> initialize();

    /// 按 context + moduleId 获取任务。
    Expected<NO<Task>> task(const std::string &category,
                            const std::string &context,
                            const std::string &id) const;

    /// 获取指定 context 下某类别的所有任务。
    Expected<std::vector<NO<Task>>> tasks(const std::string &category,
                                          const std::string &context) const;

    /// G2p 批量转换。
    std::vector<G2pRes> convert(const std::vector<G2pInput> &input);
};
```

### 3.3 数据结构

```cpp
struct G2pInput {
    std::string lyric;       // 输入文本
    std::string g2pId;       // 模块 ID
    std::string context;     // 声库名（空 = 默认 context）
};

struct G2pRes {
    std::string lyric;
    std::string g2pId;
    std::string context;     // 声库名（空 = 默认）
    std::string pronunciation;
    std::vector<std::string> candidates;
    std::string mode = "copy";
    G2pErrorType errorType = NoError;
};
```

> **v4.0 变更**：移除 `G2pInput.contextVersion` 和 `G2pRes.contextVersion`。路由仅需 context 字符串。

---

## 4. 错误检查清单

### 4.1 注册阶段：`addPackagePath(context, path)`

| # | 检查 | 错误类型 | 处理 |
|---|------|---------|------|
| R-1 | context 名含非法字符 | `ValidationError` | 返回 Error |
| R-2 | context 名超过 128 字符 | `ValidationError` | 返回 Error |
| R-3 | path 不存在 | `FileSystemError` | 返回 Error |
| R-4 | path 不是目录 | `FileSystemError` | 返回 Error |
| R-5 | 同 context 下重复添加相同 path | — | 日志 `Debug`，静默跳过 |

### 4.2 初始化阶段：`initialize()`

| # | 检查 | 处理 |
|---|------|------|
| I-1 | 某 context 搜索路径下无 package | 日志 `Warning`，不阻塞 |
| I-2 | package.json 解析失败 | 日志 `Critical`，跳过该包 |
| I-3 | 模块 config.json 读取失败 | 日志 `Critical`，跳过该模块 |
| I-4 | 模块缺少必要字段 | 日志 `Warning`，跳过该模块 |
| I-5 | moduleId 含 `:` 字符 | `ValidationError`，跳过该模块 |

**依赖解析（per-context）**：

| # | 检查 | 处理 |
|---|------|------|
| Dep-1 | context 内依赖缺失，默认 context 也无 | `DependencyError` |
| Dep-2 | 依赖回退到默认 context 成功 | 日志 `Debug` |
| Dep-3 | 依赖存在于其他非默认 context | `DependencyError`（禁止跨 context 依赖） |
| Dep-4 | 版本范围不满足 | `DependencyError` |
| Dep-5 | Level 不匹配 | `DependencyError` |
| Dep-6 | 循环依赖 | `DependencyError` |

**初始化顺序**：

| # | 检查 | 处理 |
|---|------|------|
| Ord-1 | 默认 context 失败 | 阻塞整个初始化 |
| Ord-2 | 非默认 context 失败 | 标记 Failed，不阻塞其他 context |

### 4.3 运行阶段：`task(category, context, id)`

| # | 检查 | 处理 |
|---|------|------|
| T-1 | category 为空 | `ValidationError` |
| T-2 | id 为空 | `ValidationError` |
| T-3 | context 含非法字符 | `ValidationError` |
| T-4 | category 不存在 | `RuntimeError` |
| T-5 | context 初始化失败 | `RuntimeError` |
| T-6 | context 从未注册 | `RuntimeError` |
| T-7 | 指定 context 下无此 id | `RuntimeError` |

### 4.4 运行阶段：`convert(input)`

| # | 检查 | 处理 |
|---|------|------|
| C-1 | 输入列表为空 | 返回空 vector |
| C-2 | lyric 为空 | skip，mode="skip" |
| C-3 | g2pId 为空 | copy fallback |
| C-4 | context 含非法字符 | copy fallback |
| C-5 | context 从未注册 | 日志 `Critical`，copy fallback |
| C-6 | g2pId 在指定 context 不存在 | 日志 `Critical`，copy fallback。**不做跨 context 回退** |
| C-7 | task->start() 失败 | 日志 `Critical`，copy fallback |
| C-8 | 非预期结果类型 | 日志 `Critical`，copy fallback |

> **C-6 不回退**：`convert()` 不做跨 context 回退。用户指定了 "SingerA" 就应该只在 SingerA 里找，找不到应当报错，避免静默降级。

---

## 5. 内部数据结构

### 5.1 PackageManager::Impl

```cpp
// 按 context 分组的搜索路径
std::map<std::string, llvm::SmallVector<std::filesystem::path>> contextPackagePaths;

// 按 context 分组的包索引缓存
std::map<std::string,
    std::map<std::string, std::map<stdc::VersionNumber, PackageBrief>>
> contextCachedIndexes;

// 按 context 分组的模块元数据
std::map<std::string, std::vector<ModuleMetadata>> contextModuleInfos;

// 各 context 初始化状态
enum class ContextState { Pending, Ready, Failed };
std::map<std::string, ContextState> contextStates;
```

### 5.2 Manager::Impl

```cpp
// 三层 tasks map
std::map<std::string,                      // category
    std::map<std::string,                  // context
        std::map<std::string, NO<Task>>    // moduleId → task
    >
> tasks;
```

### 5.3 ModuleMetadata

```cpp
struct ModuleMetadata {
    std::string context;        // 声库名（空 = 默认）
    std::string packageId;
    std::string moduleId;
    // ... 其余不变
};
```

### 5.4 ObjectPool 使用

```cpp
std::string fqid = context.empty() ? moduleSpec->id()
                                   : context + ":" + moduleSpec->id();
ic.addObject(fqid, task);
```

---

## 6. 核心流程

### 6.1 初始化流程

```
Manager::initialize()
  │
  ├─ Phase 1: 默认 context 初始化（必须先完成）
  │    ├─ collectModuleMetadata("", defaultPaths)
  │    ├─ 去重检查
  │    ├─ DependencyResolver::resolveAllDependencies(defaultModules)
  │    ├─ LevelCompatibilityChecker::checkAll(...)
  │    ├─ DependencyGraph::buildGraph() + findCycles()
  │    ├─ getPackageInitializationOrder()
  │    ├─ loadPackagesInOrder() + createModuleTask()
  │    └─ 失败 → 返回 Error（Ord-1），不继续
  │
  ├─ Phase 2: 其他 context 逐个初始化
  │    for each (context, paths) in contextPackagePaths where context != "":
  │    ├─ collectModuleMetadata(context, paths)
  │    ├─ 去重检查
  │    ├─ DependencyResolver::resolveAllDependencies(
  │    │      contextModules,
  │    │      fallbackModules = defaultContextModules  // ← 回退候选
  │    │  )
  │    ├─ LevelCompatibilityChecker::checkAll(...)
  │    ├─ DependencyGraph::buildGraph() + findCycles()
  │    ├─ getPackageInitializationOrder()
  │    ├─ loadPackagesInOrder() + createModuleTask()
  │    ├─ 成功 → contextStates[context] = Ready
  │    └─ 失败 → contextStates[context] = Failed，不阻塞其他
  │
  └─ Phase 3: 加载 tasks
       loadTasksForCategory("g2p") — 遍历所有 Ready context
```

### 6.2 convert() 调度

```
Manager::convert(vector<G2pInput> input)
  ├─ 过滤：跳过 lyric 为空 (C-2)、g2pId 为空 (C-3)、context 非法 (C-4) 的项
  ├─ 分组：按 (context, g2pId) 相邻分组
  └─ 对每组:
       ├─ task = impl.tasks["g2p"][context][g2pId]
       ├─ if (!task):
       │    ├─ 日志 Critical (C-5 或 C-6)
       │    └─ 生成 fallback G2pRes（mode="copy"）
       ├─ else:
       │    ├─ task->start(input)
       │    ├─ 成功 → 收集结果，填充 context 字段
       │    └─ 失败 → 日志 Critical (C-7/C-8)，生成 fallback
       └─ 每个 G2pRes.context = 输入的 context
```

---

## 7. Context 命名规范

| 规则 | 说明 |
|------|------|
| 允许字符 | `[A-Za-z0-9_.-]` |
| 禁止字符 | `:` `/` `\` `[` `]` `;` `'` `"` 空格 |
| 大小写 | 保留原样，区分大小写 |
| 最大长度 | 128 字符 |
| 空字符串 | 合法，表示默认 context |

---

## 8. 不变量

1. **默认 context 先初始化**：其他 context 的依赖回退依赖于此
2. **默认 context 全局可见**（单向）：回退只在依赖解析阶段发生
3. **convert() 不做 context 回退**：用户意图显式表达，不静默降级
4. **不同 context 之间互相不可见**
5. **同 context 内 `(moduleId, iid, level, version)` 去重**
6. **FQID 在整个系统中唯一**
7. **`:` 是保留字符**：不能出现在 context name 或 moduleId 中
8. **每个失败 context 不阻塞其他 context**（默认 context 除外）

---

## 9. 宿主调用示例

```cpp
auto langMgr = LangCore::Manager::instance();

// 官方包（默认 context）
langMgr->addPackagePath("", "/path/to/official/G2pPackages");

// SingerA 的自定义 G2p
langMgr->addPackagePath("SingerA", "/voicebanks/SingerA/g2p_packages");

// SingerB 的自定义 G2p
langMgr->addPackagePath("SingerB", "/voicebanks/SingerB/g2p_packages");

auto initResult = langMgr->initialize();

// 转换
std::vector<LangCore::G2pInput> inputs;
inputs.emplace_back("你好", "g2p-cmn-custom", "SingerA");  // 使用 SingerA 自定义 G2p
inputs.emplace_back("hello", "eng-cmu", "");                // 使用官方 G2p

auto results = langMgr->convert(inputs);
```

---

## 10. 边界情况

### 10.1 不同声库提供同名 g2p

SingerA 和 SingerB 各有 `g2p-cmn-custom`。Context 隔离，各自独立加载。

### 10.2 声库 g2p 依赖官方包

"SingerA" 的 `g2p-cmn-custom` 依赖 `cmn-official:g2p-cmn-official`。依赖解析在 SingerA context 未找到，回退到默认 context 成功。

### 10.3 没有自定义 g2p 的声库

宿主不调用 `addPackagePath`，`convert` 时传 `context=""`。

### 10.4 宿主传入不存在的 context

`convert` 中传入不存在的 context → 该项产生 copy fallback (C-5)。

---

**文档版本**: 3.1  
**最后更新**: 2026-06-30