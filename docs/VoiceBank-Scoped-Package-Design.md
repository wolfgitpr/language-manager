# Voice Bank Scoped Package 设计文档

**版本**：3.0  
**日期**：2026-04-27  
**关联 PRD**：PRD-v2.0.md §14  
**状态**：v2.1 已实现（context 隔离）。v3.0 已实现（ContextKey 版本维度）。

---

## 1. 问题陈述

### 1.1 使用场景

用户在宿主应用中安装多个声库（voice bank），每个声库可能有多个版本同时存在。每个声库的每个版本都有几率携带自定义 G2p package（自定义词典、特殊发音规则等）。这些 package 内的 `moduleId`（即 `g2pId`）可能与其他声库的 package 重复。

典型目录结构：

```
voicebanks/
├── SingerA/
│   ├── v1.0/
│   │   └── g2p_packages/
│   │       └── SingerA-G2p/          # g2pId = "g2p-cmn-custom"
│   └── v2.0/
│       └── g2p_packages/
│           └── SingerA-G2p/          # g2pId = "g2p-cmn-custom" (可能相同)
├── SingerB/
│   └── v1.0/
│       └── g2p_packages/
│           └── SingerB-G2p/          # g2pId = "g2p-cmn-custom" (和 SingerA 重复！)
```

### 1.2 现状问题

| 问题 | 现状 | 影响 |
|------|------|------|
| **g2pId 冲突** | ObjectPool 和 `impl.tasks` 使用 `moduleId` 作为 flat key | 不同声库的同名模块互相覆盖 |
| **无调用方区分** | `G2pInput.g2pId` 是 flat string | 调用方无法指定使用哪个声库的 G2p |
| **版本冗余** | 模块去重不含 packageId | 同声库不同版本若内容没变，第二个被静默跳过，无明确共用语义 |
| **无声库概念** | `addPackagePath` 只接受路径 | 框架不知道哪些包属于哪个声库 |

### 1.3 设计目标

1. 不同声库用声库名区分，同名 g2pId 不冲突
2. 调用时携带声库名，精确路由
3. 同声库不同版本去重：`(g2pId, version)` 相同只加载一份，发出日志
4. 每个环节都有完善的错误检查和报告
5. 不保留旧接口——API 整体升级

---

## 2. 核心概念：Context

**Context**（上下文）= 声库名或任意用户定义的命名空间，作为 package 的归属标识。

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
| 四元组 `(moduleId, iid, level, version)` 完全相同 | **共用**：只保留首次加载的，日志 `Information`："Module 'X' v1.0 (class Y, level Z) in context 'SingerA' already loaded from 'path1', skipping duplicate from 'path2'" |
| `moduleId` 相同但四元组不完全相同 | **共存**：都加载，`selectBestModules` 照常工作 |

> 去重的 key 是 `(context, moduleId, iid, level, version)` 而非仅 `(context, moduleId, version)`，因为同一个 moduleId 在不同 plugin class（iid）或不同 level 下是不同的模块。

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
    /// @param context 声库名（空字符串 = 默认 context）
    /// @param path 包搜索目录
    /// @return 成功或错误（ValidationError: context 名非法; FileSystemError: 路径不存在）
    Expected<void> addPackagePath(const std::string &context,
                                  const std::filesystem::path &path);

    /// 在指定 context 下批量设置包搜索路径（替换该 context 已有路径）。
    /// @return 成功或第一个失败的错误
    Expected<void> setPackagePaths(const std::string &context,
                                   const std::vector<std::filesystem::path> &paths);

    /// 获取指定 context 的包搜索路径。
    /// 若 context 不存在返回空 vector（不报错）。
    std::vector<std::filesystem::path> packagePaths(const std::string &context) const;

    /// 获取所有已注册的 context 名（含默认 context ""）。
    std::vector<std::string> contexts() const;
};
```

### 3.2 Manager

```cpp
class Manager : public PackageManager {
public:
    /// 初始化。
    /// @return 成功或错误（含所有 context 的聚合错误信息）
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
/// G2p 转换输入
struct G2pInput {
    std::string lyric;       // 输入文本
    std::string g2pId;       // 模块 ID
    std::string context;     // 声库名（空 = 默认 context）
};

/// G2p 转换结果
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

> `G2pInput` 已包含 `context` 字段。v3.0 进一步增加了 `contextVersion` 字段。

---

## 4. 错误检查清单

本节按调用流程逐环节列出所有需要检查的错误条件。

### 4.1 注册阶段：`addPackagePath(context, path)`

| # | 检查 | 错误类型 | 错误消息 | 处理 |
|---|------|---------|---------|------|
| R-1 | context 名含非法字符 | `ValidationError` | "Invalid context name 'X': contains forbidden character 'Y'. Allowed: [A-Za-z0-9_.-]" | 返回 Error，不注册 |
| R-2 | context 名超过 128 字符 | `ValidationError` | "Context name 'X...' exceeds maximum length (128)" | 返回 Error，不注册 |
| R-3 | path 不存在 | `FileSystemError` | "Package path does not exist: 'path'" | 返回 Error，不注册 |
| R-4 | path 不是目录 | `FileSystemError` | "Package path is not a directory: 'path'" | 返回 Error，不注册 |
| R-5 | path 无法 canonicalize（权限等） | `FileSystemError` | "Cannot resolve package path 'path': OS error" | 返回 Error，不注册 |
| R-6 | 同 context 下重复添加相同 path | — | 日志 `Debug`："Path 'X' already registered for context 'Y', skipping" | 静默跳过，返回成功 |

### 4.2 初始化阶段：`initialize()`

#### 4.2.1 包发现与元数据收集

| # | 检查 | 错误类型 | 处理 |
|---|------|---------|------|
| I-1 | 某个 context 的搜索路径下无任何 package 子目录 | — | 日志 `Warning`："No packages found in context 'X' (paths: [...])"。不阻塞，该 context 无可用模块 |
| I-2 | package.json 解析失败 | `ConfigError` | 日志 `Critical`，跳过该包，错误加入聚合列表 |
| I-3 | 模块 config.json 读取失败 | `ConfigError` | 日志 `Critical`，跳过该模块，错误加入聚合列表 |
| I-4 | 模块缺少必要字段（moduleId/class 为空） | `ConfigError` | 日志 `Warning`，跳过该模块 |
| I-5 | moduleId 含 `:` 字符 | `ValidationError` | 日志 `Critical`："Module ID 'X' in package 'Y' contains ':' which is reserved for context separation"。跳过该模块 |
| I-6 | config.json 中 `level <= 0` | `ConfigError` | 返回错误（已在 P0 修复中实现） |

#### 4.2.2 同 Context 去重

| # | 检查 | 日志级别 | 消息 |
|---|------|---------|------|
| D-1 | 同 context 内 `(moduleId, iid, level, version)` 完全相同 | `Information` | "Module 'X' v1.0 (class Y, level Z) in context 'C' already loaded from 'path1', skipping duplicate from 'path2'" |
| D-2 | 同 context 内 `moduleId` 相同但 `(iid, level, version)` 不完全相同 | `Debug` | "Module 'X' in context 'C': loading variant (class Y2, level Z2, v2.0) alongside existing (class Y1, level Z1, v1.0)" |

#### 4.2.3 依赖解析（per-context）

| # | 检查 | 错误类型 | 处理 |
|---|------|---------|------|
| Dep-1 | context 内依赖缺失，默认 context 也无 | `DependencyError` | "Dependency 'pkg:mod' not found for module 'X' in context 'C'. Searched in context 'C' and default context." |
| Dep-2 | 依赖回退到默认 context 时成功 | — | 日志 `Debug`："Dependency 'pkg:mod' for module 'X' in context 'C' resolved via default context" |
| Dep-3 | 依赖指向的 packageId 存在于其他非默认 context | `DependencyError` | "Dependency 'pkg:mod' for module 'X' in context 'C' not found. Note: a matching module exists in context 'D' but cross-context dependencies are not allowed." |
| Dep-4 | 版本范围不满足 | `DependencyError` | 同现有错误消息，附加 " (in context 'C')" |
| Dep-5 | Level 不匹配 | `DependencyError` | 同现有，附加 context 信息 |
| Dep-6 | 循环依赖（context 内） | `DependencyError` | "Circular dependency detected in context 'C': [A → B → A]" |
| Dep-7 | 循环依赖涉及默认 context 回退 | `DependencyError` | "Circular dependency detected involving context 'C' and default context: [C:A → :B → C:A]" |
| Dep-8 | 自依赖 | `DependencyError` | 同现有，附加 context 信息 |

#### 4.2.4 Level 兼容性检查

| # | 检查 | 处理 |
|---|------|------|
| Lv-1 | 某 context 的模块 level 超出 [min, max] | `DependencyError`，附加 context 信息。该 context 整体标记为失败 |

#### 4.2.5 初始化顺序与加载

| # | 检查 | 处理 |
|---|------|------|
| Ord-1 | 默认 context 解析/加载失败 | **阻塞整个初始化**——其他 context 可能依赖默认 context，继续加载无意义。`initialize()` 返回聚合错误 |
| Ord-2 | 某个非默认 context 解析/加载失败 | 日志 `Critical`，该 context 标记为不可用，**不阻塞**其他 context。错误加入聚合列表 |
| Ord-3 | 所有 context 都失败 | `initialize()` 返回聚合错误 |
| Ord-4 | 插件 DLL 加载失败 | 同现有处理，附加 context 信息 |
| Ord-5 | Task::initialize() 失败 | 同现有处理，附加 context 信息 |

#### 4.2.6 初始化结果聚合

`initialize()` 返回 `Expected<void>`。失败时，Error 消息聚合所有 context 的错误：

```
Failed to initialize Language Manager:

[Default context]
  - OK (7 modules loaded)

[Context 'SingerA']
  - Module 'g2p-cmn-custom': dependency 'cmn-official:g2p-cmn-official' not found
  - Module 'g2p-cmn-chain': initialization failed: model file not found

[Context 'SingerB']
  - OK (2 modules loaded)

1 context(s) failed. 2 context(s) succeeded.
```

### 4.3 运行阶段：`task(category, context, id)`

| # | 检查 | 错误类型 | 错误消息 |
|---|------|---------|---------|
| T-1 | `category` 为空 | `ValidationError` | "Category cannot be empty" |
| T-2 | `id` 为空 | `ValidationError` | "Module ID cannot be empty" |
| T-3 | `context` 含非法字符 | `ValidationError` | "Invalid context name: ..." |
| T-4 | `category` 不存在 | `RuntimeError` | "Unknown category 'X'. Available: g2p, driver, dict" |
| T-5 | `context` 存在但该 context 初始化失败 | `RuntimeError` | "Context 'X' is not available (initialization failed). Check initialization logs." |
| T-6 | `context` 从未注册 | `RuntimeError` | "Unknown context 'X'. Available contexts: ['', 'SingerA', 'SingerB']" |
| T-7 | 指定 context 下无此 id | `RuntimeError` | "Module 'X' not found in context 'Y' (category 'Z'). Available modules in this context: [...]" |

### 4.4 运行阶段：`convert(input)`

| # | 检查 | 处理 |
|---|------|------|
| C-1 | 输入列表为空 | 返回空 `vector<G2pRes>` |
| C-2 | 某项 `lyric` 为空 | 日志 `Warning`，该项产生 `G2pRes{lyric="", mode="copy", errorType=InvalidLyric}` |
| C-3 | 某项 `g2pId` 为空 | 日志 `Warning`，该项产生 `G2pRes{mode="copy", errorType=UnknownError}` |
| C-4 | 某项 `context` 含非法字符 | 日志 `Warning`，该项产生 `G2pRes{mode="copy", errorType=UnknownError}`（不中断其他项） |
| C-5 | 指定 context 下无此 g2pId，默认 context 也无 | 日志 `Critical`，该组产生 fallback `G2pRes{mode="copy"}` |
| C-6 | 指定 context 下无此 g2pId，默认 context 有 | **不回退**。日志 `Critical`："Module 'X' not found in context 'Y'. Use context '' to use the default module." 产生 fallback。**理由**：convert 阶段不做隐式回退，避免用户误以为使用了声库自定义 G2p 但实际用了官方的。回退仅在依赖解析阶段发生。 |
| C-7 | task->start() 返回错误 | 日志 `Critical`，该组产生 fallback `G2pRes{mode="copy"}`，`errorType` 根据错误类型设置 |
| C-8 | task->start() 返回非预期结果类型 | 日志 `Critical`，该组产生 fallback |

> **关键决策 C-6**：`convert()` 不做 context 回退，与 `依赖解析` 的回退策略不同。理由：依赖解析是框架内部行为（声库自定义 G2p 依赖官方模块是合理的），而 `convert()` 是用户意图的直接表达——用户指定了 "SingerA" 就应该只在 SingerA 里找，找不到应当报错，让用户明确改为默认 context。这避免了静默降级。

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

// context 内模块去重追踪
// key = (context, moduleId, iid, level, version)
struct ModuleDeduplicationKey {
    std::string context;
    std::string moduleId;
    std::string iid;
    int level;
    std::string version;
    bool operator==(const ModuleDeduplicationKey &) const = default;
};
struct ModuleDeduplicationKeyHash { size_t operator()(const ModuleDeduplicationKey &) const; };
std::unordered_map<ModuleDeduplicationKey, std::filesystem::path, ModuleDeduplicationKeyHash>
    loadedModules;  // value = first-seen source path

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

### 5.3 ModuleMetadata 变更

```cpp
struct ModuleMetadata {
    std::string context;        // 新增
    std::string packageId;
    std::string moduleId;
    // ... 其余不变
    
    // key() 需加入 context
    std::string key() const {
        return context + ":" + packageId + ":" + moduleId + ":" + version + ":"
             + iid + ":" + type + ":" + configuration + ":" + std::to_string(level);
    }
};
```

### 5.4 ObjectPool 使用

`addObject` 的 key 改为 FQID：

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
  │    ├─ 去重检查（D-1, D-2）
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
  │    ├─ 去重检查（D-1, D-2）
  │    ├─ DependencyResolver::resolveAllDependencies(
  │    │      contextModules,
  │    │      fallbackModules = defaultContextModules  // ← 回退候选
  │    │  )
  │    ├─ LevelCompatibilityChecker::checkAll(...)
  │    ├─ DependencyGraph::buildGraph() + findCycles()
  │    ├─ getPackageInitializationOrder()
  │    ├─ loadPackagesInOrder() + createModuleTask()
  │    ├─ 成功 → contextStates[context] = Ready
  │    └─ 失败 → contextStates[context] = Failed，日志 Critical，
  │              错误加入聚合列表，继续下一个 context
  │
  └─ Phase 3: 加载 tasks
       loadTasksForCategory("g2p") — 遍历所有 Ready context，注册到三层 map
       返回 Expected<void>（含所有失败 context 的聚合错误，或成功）
```

### 6.2 依赖解析中的 Context 回退

`DependencyResolver::resolveAllDependencies` 新增可选参数 `fallbackModules`：

```
对每个待解析模块的每个依赖:
  1. 在本 context 的候选列表中按 (packageId, moduleId, level, version) 查找
  2. 找到 → 使用
  3. 未找到且 fallbackModules 非空 → 在 fallbackModules 中查找
     3a. 找到 → 使用，日志 Debug (Dep-2)
     3b. 检查该依赖是否存在于其他已知 context → 若是，报 Dep-3（明确告知跨 context 不可）
     3c. 都没有 → 报 Dep-1
```

### 6.3 convert() 调度

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
| 验证位置 | `addPackagePath` 入口 (R-1, R-2)，`task()` 入口 (T-3)，`convert()` 每项 (C-4) |
| moduleId 校验 | 加载时校验 moduleId 不含 `:` (I-5) |

---

## 8. 边界情况

### 8.1 不同声库提供同名 g2p，配置不同

SingerA 和 SingerB 各有 `g2p-cmn-custom v1.0` 但词典不同。Context 隔离，各自独立加载。

### 8.2 同声库多版本，g2p 版本号不同

SingerA v1.0 携带 `g2p-cmn-custom v1.0`，v2.0 携带 `v2.0`。两者都加载到 "SingerA" context，`selectBestModules` 保留 v2.0。

### 8.3 声库 g2p 依赖官方包

"SingerA" 的 `g2p-cmn-custom` 依赖 `cmn-official:g2p-cmn-official`。依赖解析在 SingerA context 未找到，回退到默认 context 成功。

### 8.4 声库 g2p 依赖同声库另一模块

在同一 context 内解析，不涉及回退。

### 8.5 没有自定义 g2p 的声库

宿主不调用 `addPackagePath`，`convert` 时传 `context=""`。

### 8.6 默认 context 为空（未加载官方包）

无官方模块。其他 context 的依赖回退全部失败 (Dep-1)。`initialize()` 返回的错误信息中默认 context 标注 "0 modules loaded"。

### 8.7 宿主传入不存在的 context

`task("g2p", "NonExist", "g2p-x")` → Error (T-6)，消息列出可用 context。  
`convert` 中传入不存在的 context → 该项产生 fallback (C-5)。

### 8.8 声库 g2p 的 moduleId 与官方包的 moduleId 重名

例如 SingerA 也有一个叫 `g2p-cmn-official` 的模块。在 SingerA context 内它覆盖不了默认 context 的模块（隔离的）。`convert("...", "g2p-cmn-official", "SingerA")` 使用 SingerA 的，`convert("...", "g2p-cmn-official", "")` 使用官方的。不冲突。但依赖解析时，SingerA context 内的其他模块若依赖 `g2p-cmn-official`，会优先使用 SingerA 自己的（本 context 优先）。

### 8.9 同 context 内同 moduleId+iid+level 但 version 不同

两个版本都加载，`selectBestModules` 保留最高版本。日志 Debug (D-2)。最终该 context 内只有一个 task 实例。

---

## 9. 不变量

1. **默认 context 先初始化**：其他 context 的依赖回退依赖于此
2. **默认 context 全局可见**（单向）：回退只在依赖解析阶段发生
3. **convert() 不做 context 回退**：用户意图显式表达，不静默降级
4. **不同 context 之间互相不可见**
5. **同 context 内 `(moduleId, iid, level, version)` 去重**
6. **FQID 在整个系统中唯一**
7. **`:` 是保留字符**：不能出现在 context name 或 moduleId 中
8. **每个失败 context 不阻塞其他 context**（默认 context 除外）

---

## 10. 与现有子系统的交互

| 子系统 | 变更 |
|--------|------|
| **DependencyResolver** | `resolveAllDependencies()` 新增 `fallbackModules` 参数；候选过滤增加 context 维度 |
| **DependencyGraph** | `ModuleMetadata.key()` 加入 context 前缀；节点在 context 内构建 |
| **LevelCompatibilityChecker** | 无变更——Level 检查全局 |
| **ObjectPool / ModuleCategory** | key 从 moduleId → FQID |
| **PluginFactory** | 无变更——插件不感知 context |
| **Plugin / Task** | 无变更——不感知 context |
| **package.json** | 无变更——context 是运行时概念 |

---

## 11. 实现影响评估

| 组件 | 变更量 | 说明 |
|------|--------|------|
| **PackageManager.h** | 中 | API 改为带 context，返回 Expected |
| **PackageManager_p.h** | 中 | 数据结构改 context 分组，新增 contextStates |
| **PackageManager.cpp** | 大 | 扫描/索引/加载逻辑全面 context 化 |
| **Manager.h** | 中 | API 全部带 context，删除旧接口 |
| **Manager.cpp** | 中 | tasks 三层 map，convert() context 调度 |
| **LangCommon.h** | 中 | 删除 G2pInput，新增 G2pConvertInput，G2pRes 加 context |
| **DependencyGraph.h** | 小 | ModuleMetadata 加 context 字段 |
| **DependencyResolver.cpp** | 中 | fallbackModules 参数，跨 context 检测 (Dep-3) |

---

## 12. 不做的事

| 不做 | 理由 |
|------|------|
| 跨 Context 依赖 | 声库间不应有 G2p 依赖 |
| convert() 隐式回退 | 用户意图应显式，避免静默降级 |
| 运行时动态加卸载 | Manager 不支持 re-initialize，独立课题 |
| package.json 内声明 context | 运行时概念，由宿主指定 |
| Context 嵌套 | 无实际需求 |
| 旧接口保留 | 项目未发布，不需向后兼容 |

---

# v3.0 扩展：Context 版本维度（已实现）

---

## 13. 问题陈述

### 13.1 v2.1 遗留问题

v2.1 通过 context（声库名）实现了不同声库间的命名空间隔离。但同一声库的不同版本共享同一个 context，导致：

| 问题 | 场景 | 影响 |
|------|------|------|
| **版本覆盖** | SingerA v1.0 携带 `g2p-cmn-custom v1.0`，v2.0 携带 `v2.0`。两者注册到 `context="SingerA"` | `selectBestModules` 只保留 v2.0，v1.0 的 G2p 永远不会被使用 |
| **无法并存** | 宿主同时打开 SingerA v1.0 和 v2.0 的工程 | 无法为不同版本的声库路由到各自的 G2p |
| **静默降级** | 用户切换声库版本后，旧版本的发音规则消失 | 导致不可预期的发音变化，用户无感知 |

### 13.2 设计目标

1. 同一声库的不同版本可各自携带独立的 G2p，互不干扰
2. 调用时可精确指定声库版本，路由到对应 G2p
3. 版本为空时退化为现有行为（向后兼容）
4. 默认 context（`""`）不受影响，仍为无版本语义
5. 版本信息不编码在 context 字符串内，而是作为独立的结构化字段

---

## 14. 核心设计：ContextKey

引入 `ContextKey` 作为复合键，替代所有内部以 `std::string context` 作为 map key 的位置。

### 14.1 定义

```cpp
// ContextUtils.h

struct ContextKey {
    std::string context;                // 声库名，空字符串 = 默认 context
    stdc::VersionNumber version;        // 声库版本，isNull() = 无版本

    bool operator<(const ContextKey &o) const {
        if (context != o.context) return context < o.context;
        return version < o.version;
    }

    bool operator==(const ContextKey &o) const {
        return context == o.context && version == o.version;
    }

    bool operator!=(const ContextKey &o) const { return !(*this == o); }

    /// 无版本标识
    bool isVersioned() const { return !version.isNull(); }

    /// 是否为默认 context
    bool isDefault() const { return context.empty() && version.isNull(); }

    /// 人可读表示：
    ///   "" → "(default)"
    ///   "SingerA" → "SingerA"
    ///   "SingerA" + 2.0.0 → "SingerA@2.0.0"
    std::string toString() const {
        if (context.empty() && version.isNull()) return "(default)";
        if (version.isNull()) return context;
        return context + "@" + version.toString();
    }
};
```

**设计决策**：
- 使用 `stdc::VersionNumber` 与 dsinfer 规范对齐（`Package::version()` 已使用此类型）
- `@` 作为 version 分隔符，不在 context 合法字符 `[A-Za-z0-9_.-]` 中，天然无歧义
- `ContextKey` 是值类型，支持 `<` 和 `==`，可直接作为 `std::map` 的 key

### 14.2 FQID 扩展

```
无版本:    context + ":" + moduleId         → "SingerA:g2p-cmn-custom"
带版本:    context + "@" + version + ":" + moduleId → "SingerA@2.0.0:g2p-cmn-custom"
默认ctx:   moduleId                          → "g2p-cmn-official"
```

```cpp
// ContextUtils 更新
static std::string formatFqid(const ContextKey &ctxKey, const std::string_view &moduleId) {
    if (ctxKey.isDefault())
        return std::string(moduleId);
    return ctxKey.toString() + ":" + std::string(moduleId);
}

static FqidParseResult parseFqid(const std::string_view &fqid) {
    // 先找 ':'，分出 contextPart 和 moduleId
    // 再在 contextPart 中找 '@'，分出 context 和 version
    // "SingerA@2.0.0:g2p-cmn" → {context="SingerA", version=2.0.0, moduleId="g2p-cmn"}
    // "SingerA:g2p-cmn" → {context="SingerA", version=null, moduleId="g2p-cmn"}
    // "g2p-cmn" → {context="", version=null, moduleId="g2p-cmn"}
}
```

`FqidParseResult` 扩展：
```cpp
struct FqidParseResult {
    std::string context;
    stdc::VersionNumber version;    // 新增
    std::string moduleId;
};
```

---

## 15. API 变更

### 15.1 PackageManager

```cpp
class PackageManager {
public:
    /// 在指定 context + version 下添加包搜索路径。
    /// @param context 声库名（空 = 默认 context）
    /// @param version 声库版本（isNull = 无版本）
    /// @param path 包搜索目录
    Expected<void> addPackagePath(const std::string &context,
                                  const stdc::VersionNumber &version,
                                  const std::filesystem::path &path);

    /// 向后兼容重载：无版本
    Expected<void> addPackagePath(const std::string &context,
                                  const std::filesystem::path &path);

    /// 批量设置（带版本）
    Expected<void> setPackagePaths(const std::string &context,
                                   const stdc::VersionNumber &version,
                                   const std::vector<std::filesystem::path> &paths);

    /// 向后兼容重载
    Expected<void> setPackagePaths(const std::string &context,
                                   const std::vector<std::filesystem::path> &paths);

    /// 获取路径
    std::vector<std::filesystem::path> packagePaths(const std::string &context,
                                                     const stdc::VersionNumber &version = {}) const;

    /// 获取所有已注册的 ContextKey
    std::vector<ContextKey> contextKeys() const;

    /// 保留：获取所有不重复的 context 名
    std::vector<std::string> contexts() const;
};
```

**向后兼容策略**：

```cpp
Expected<void> PackageManager::addPackagePath(const std::string &context,
                                               const std::filesystem::path &path) {
    return addPackagePath(context, {}, path);  // 空版本
}
```

### 15.2 Manager

```cpp
class Manager : public PackageManager {
public:
    /// 按 ContextKey + moduleId 获取任务（带版本）
    Expected<NO<Task>> task(const std::string &category,
                            const std::string &context,
                            const stdc::VersionNumber &version,
                            const std::string &id) const;

    /// 向后兼容重载
    Expected<NO<Task>> task(const std::string &category,
                            const std::string &context,
                            const std::string &id) const;

    /// 获取某 ContextKey 下某类别的所有任务
    Expected<std::vector<NO<Task>>> tasks(const std::string &category,
                                           const std::string &context,
                                           const stdc::VersionNumber &version = {}) const;

    /// G2p 批量转换
    std::vector<G2pRes> convert(const std::vector<G2pInput> &input);
};
```

### 15.3 数据结构

```cpp
// LangCommon.h

struct G2pInput {
    std::string lyric;
    std::string g2pId;
    std::string context;
    stdc::VersionNumber contextVersion;   // 新增；default-constructed = 无版本

    G2pInput() = default;
    G2pInput(std::string lyric, std::string g2pId, std::string context = "",
             stdc::VersionNumber contextVersion = {})
        : lyric(std::move(lyric)), g2pId(std::move(g2pId)),
          context(std::move(context)), contextVersion(std::move(contextVersion)) {}
};

struct G2pRes {
    std::string lyric;
    std::string g2pId;
    std::string context;
    stdc::VersionNumber contextVersion;   // 新增
    std::string pronunciation;
    std::vector<std::string> candidates;
    std::string mode = "copy";
    G2pErrorType errorType = NoError;

    // 构造函数同步更新（保持向后兼容的默认参数）
};
```

### 15.4 ModuleMetadata 扩展

```cpp
struct ModuleMetadata {
    std::string context;
    stdc::VersionNumber contextVersion;   // 新增
    std::string packageId;
    std::string moduleId;
    // ... 其余不变

    // key() 加入 contextVersion
    std::string key() const {
        std::string ctxPart = context;
        if (!contextVersion.isNull())
            ctxPart += "@" + contextVersion.toString();
        return ctxPart + ":" + packageId + ":" + moduleId + ":"
             + version + ":" + iid + ":" + type + ":" + configuration
             + ":" + std::to_string(level);
    }

    // uniqueKey(), isSameMainModule() 同步更新，加入 contextVersion
};
```

---

## 16. 内部存储变更

### 16.1 PackageManager_p.h

所有以 `std::string` (context) 为 key 的 map → 改为 `ContextKey`：

```cpp
// 按 ContextKey 分组的搜索路径
std::map<ContextKey, llvm::SmallVector<std::filesystem::path>> contextPackagePaths;

// 按 ContextKey 分组的包索引缓存
std::map<ContextKey, std::map<std::string, std::map<stdc::VersionNumber, PackageBrief>, std::less<>>>
    contextCachedIndexes;

// 按 ContextKey 分组的模块元数据
std::map<ContextKey,
         std::unordered_set<ModuleMetadata, ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>>
    contextModuleInfoSets;
std::map<ContextKey, std::vector<ModuleMetadata>> contextModuleInfos;

// 各 ContextKey 初始化状态
std::map<ContextKey, ContextState> contextStates;
```

### 16.2 Manager_p.h

```cpp
// 三层 map 保持三层，但第二层 key 从 string → ContextKey
std::map<std::string,                           // category
    std::map<ContextKey,                        // context + version
        std::map<std::string, NO<Task>>         // moduleId → Task
    >
> tasks;
```

**关键决策**：保持三层 map 而非四层。ContextKey 作为单一复合键，避免 `category → context → version → moduleId` 四层嵌套带来的回退遍历复杂度。

---

## 17. 查找与路由策略

### 17.1 task() / convert() 查找链

```
输入: category, context, version, moduleId

Step 1: 精确匹配
  key = ContextKey{context, version}
  lookup tasks[category][key][moduleId]
  → 找到: 返回

Step 2: 退化到无版本（仅当 version 非空时）
  key = ContextKey{context, {}}
  lookup tasks[category][key][moduleId]
  → 找到: 返回

Step 3: 失败
  → 返回 Error / 生成 fallback G2pRes
  （不回退到默认 context，保持 C-6 不变）
```

**设计理由**：
- Step 2 覆盖"声库未注册版本化路径，只注册了不带版本的路径"的兼容场景
- 不做 "找最高兼容版本" — 版本匹配是宿主(dsinfer)的责任，框架只做精确路由
- 不做跨 context 回退（已有 C-6 决策）

### 17.2 convert() 分组

```cpp
// 分组 key 从 (context, g2pId) → (context, contextVersion, g2pId)
struct Group {
    std::string context;
    stdc::VersionNumber contextVersion;
    std::string g2pId;
    std::vector<std::string> lyrics;
    std::vector<size_t> resultIndexes;
};

// 相邻分组判定
if (groups.empty()
    || groups.back().context != item.context
    || groups.back().contextVersion != item.contextVersion
    || groups.back().g2pId != item.g2pId) {
    groups.push_back({item.context, item.contextVersion, item.g2pId, {}, {}});
}
```

### 17.3 initialize() 流程

无本质变更，只是遍历 `contextPackagePaths` 时的 key 从 `string` 变为 `ContextKey`：

```
Phase 1: ContextKey{"", {}} (默认 context，无版本)
  → 与现有完全一致

Phase 2: 所有非默认 ContextKey
  for each (ctxKey, _) in contextPackagePaths where !ctxKey.isDefault():
    → collectModuleMetadata 传入 ctxKey（context + version 都写入 ModuleMetadata）
    → 依赖解析时，fallback 仍为默认 context 的模块
    → 其余流程不变
```

### 17.4 依赖解析

`DependencyResolver::resolveAllDependencies` 已有 `fallbackModules` 参数，无需变更。每个 ContextKey 独立解析，fallback 来自默认 context。

`ModuleMetadata.contextVersion` 参与 `isSameMainModule` 判定（同一 ContextKey 内才去重），但不参与依赖匹配（依赖按 packageId + moduleId + level + versionRange 匹配，与 contextVersion 无关）。

---

## 18. 错误检查增补

在 v2.1 错误清单基础上增加：

### 18.1 注册阶段

| # | 检查 | 处理 |
|---|------|------|
| R-7 | version 格式非法（非空但无法解析） | `ValidationError`："Invalid context version 'X': expected format x.y[.z.w]" |
| R-8 | 默认 context 带版本（`context=""` 但 `version` 非空） | `ValidationError`："Default context cannot have a version" |

### 18.2 运行阶段

| # | 检查 | 处理 |
|---|------|------|
| T-8 | 指定 context + version 不存在，但 context 无版本注册存在 | 使用无版本回退（Step 2），日志 `Debug` |
| T-9 | 指定 context + version 不存在，无版本也不存在 | `RuntimeError`："Context 'SingerA@2.0.0' not found. Available: ['SingerA', 'SingerA@1.0.0', 'SingerA@2.0.0']" |
| C-9 | convert 项的 contextVersion 格式异常 | 日志 `Warning`，该项产生 fallback |

---

## 19. 边界情况增补

### 19.1 同声库不同版本，相同 g2pId

SingerA v1.0 有 `g2p-cmn-custom v1.0`，v2.0 有 `g2p-cmn-custom v2.0`。

```cpp
mgr->addPackagePath("SingerA", VersionNumber::fromString("1.0.0"), pathV1);
mgr->addPackagePath("SingerA", VersionNumber::fromString("2.0.0"), pathV2);
```

初始化后：
- `tasks["g2p"][{"SingerA", 1.0.0}]["g2p-cmn-custom"]` → v1.0 Task
- `tasks["g2p"][{"SingerA", 2.0.0}]["g2p-cmn-custom"]` → v2.0 Task

互不干扰。

### 19.2 声库只注册了无版本路径，调用时带版本

```cpp
mgr->addPackagePath("SingerA", pathV1);  // 无版本
// ...
mgr->convert({{"你好", "g2p-cmn-custom", "SingerA", VersionNumber::fromString("1.0.0")}});
```

查找链：`{"SingerA", 1.0.0}` 不存在 → 退化 `{"SingerA", {}}` → 命中。

### 19.3 声库注册了带版本路径，调用时不带版本

```cpp
mgr->addPackagePath("SingerA", VersionNumber::fromString("2.0.0"), pathV2);
// ...
mgr->convert({{"你好", "g2p-cmn-custom", "SingerA"}});
```

查找链：`{"SingerA", {}}` 不存在 → 失败（不会遍历所有版本选最高）。

**设计理由**：如果宿主知道有版本化的包，就应该传版本。不传版本意味着"使用无版本注册的包"，而非"帮我选一个"。这避免了不确定行为。

### 19.4 同声库同版本注册多个路径

与 v2.1 行为一致：同一 ContextKey 下可以有多个搜索路径，模块按现有规则去重和加载。

### 19.5 默认 context 不受影响

```cpp
mgr->addPackagePath("", officialPath);  // 默认 context，无版本
// addPackagePath("", someVersion, path) → R-8 错误
```

---

## 20. 不变量更新

在 v2.1 不变量（§9）基础上增加：

9. **ContextKey 是完整路由键**：`(context, version)` 二元组唯一确定一个命名空间
10. **版本回退仅限 versioned → unversioned**：不做跨版本选择（`v2.0` 找不到不会退化到 `v1.0`）
11. **默认 context 永远无版本**：`ContextKey{"", non-null}` 非法
12. **版本匹配是宿主职责**：框架不实现 `compatVersion` 匹配，只做精确查找 + 无版本退化

---

## 21. 变更影响总结

| 组件 | 变更量 | 说明 |
|------|--------|------|
| **ContextUtils.h** | 中 | 新增 `ContextKey`，扩展 `FqidParseResult`，更新 `formatFqid` / `parseFqid` |
| **LangCommon.h** | 小 | `G2pInput` / `G2pRes` 增加 `contextVersion` 字段 |
| **PackageManager.h** | 小 | 新增带 version 的 API 重载，`contextKeys()` |
| **PackageManager_p.h** | 中 | 所有 `map<string, ...>` → `map<ContextKey, ...>` |
| **PackageManager.cpp** | 中 | `addPackagePath` / `setPackagePaths` / `getModuleMetadatas` / `scanPackageDirectory` 参数传播 ContextKey |
| **Manager.h** | 小 | 新增带 version 的 `task()` 重载 |
| **Manager_p.h** | 小 | tasks map 第二层 key → ContextKey |
| **Manager.cpp** | 中 | `initialize()` 遍历 ContextKey，`convert()` 分组加 version，`task()` 两步查找链 |
| **DependencyGraph.h** | 小 | `ModuleMetadata` 增加 `contextVersion`，`key()` / `isSameMainModule()` 更新 |
| **DependencyResolver.cpp** | 无 | 无变更（已按 fallbackModules 工作，不感知 contextVersion） |
| **Tests (tst_context)** | 中 | 新增版本化 context 测试用例，现有测试不修改（向后兼容） |

---

## 22. 宿主调用示例

```cpp
#include <LangCore/Core/Manager.h>

auto langMgr = LangCore::Manager::instance();

// 官方包（默认 context，无版本）
langMgr->addPackagePath("", "/path/to/official/G2pPackages");

// SingerA v1.0 的自定义 G2p
langMgr->addPackagePath("SingerA",
    stdc::VersionNumber::fromString("1.0.0"),
    "/voicebanks/SingerA/v1.0/g2p_packages");

// SingerA v2.0 的自定义 G2p
langMgr->addPackagePath("SingerA",
    stdc::VersionNumber::fromString("2.0.0"),
    "/voicebanks/SingerA/v2.0/g2p_packages");

auto initResult = langMgr->initialize();

// 转换：v1.0 和 v2.0 可以并存使用
std::vector<LangCore::G2pInput> inputs;
inputs.emplace_back("你好", "g2p-cmn-custom", "SingerA",
    stdc::VersionNumber::fromString("1.0.0"));  // 使用 v1.0 的 G2p
inputs.emplace_back("世界", "g2p-cmn-custom", "SingerA",
    stdc::VersionNumber::fromString("2.0.0"));  // 使用 v2.0 的 G2p
inputs.emplace_back("hello", "eng-cmu", "");     // 使用官方 G2p

auto results = langMgr->convert(inputs);
```

---

**文档版本**: 3.0  
**最后更新**: 2026-04-27
