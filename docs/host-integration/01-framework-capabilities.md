# 01 · 框架能力审计

本文档审计 LangCore 框架对外暴露的核心能力，作为宿主集成与测试设计的能力基线。所有引用均基于已核实的代码事实。

> 2026-07-02 修订：修正 `addPluginPath` 参数（IID 非 category 名）、`initialize()` 幂等行为（返回 Error 非 no-op）、`addPackagePath` 后置行为（静默注册非 no-op）、tagger/splitter 分类（已移至前端）、ModelStep S5 回退（裸 moduleId 非 `:g2pId`）。

## 1. Manager 单例

`Manager` 是 LangCore 的核心单例，统一管理插件路径、包路径、初始化与转换入口。

**头文件**：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`

### 1.1 核心 API

| 方法 | 签名要点 | 说明 |
| --- | --- | --- |
| `instance()` | 静态单例获取 | 返回 `Manager` 单例实例（`Manager.h:23`） |
| `addPluginPath(iid, path)` | 添加插件搜索路径 | **按 IID**（`org.openvpi.Driver` / `org.openvpi.Task`）注册插件目录，非 category 名 |
| `addPackagePath(context, path)` | 添加 G2P 包路径 | `context=""` 表示官方默认上下文；version 隐含为空 |
| `addPackagePath(context, version, path)` | 添加带版本的 G2P 包路径 | 私有上下文必须传非空 version（R-8） |
| `initialize()` | 完成初始化 | **success-gated 幂等**：成功后重调返回 `Error::AlreadyInitialized`；失败可重试 |
| `initialized()` | 公共查询方法 | 返回是否已完成初始化（`Manager.h:27`） |
| `convert(inputs)` | 批量 G2P 转换 | 入参为 `vector<G2pInput>`，返回 `vector<G2pRes>`；**框架本身无回退策略** |
| `category(name)` | 获取分类对象 | 继承自 `PackageManager`，用于向分类注册对象（如 ONNX 驱动注册到 `driver` 分类） |
| `task(category, context, [version], id)` | 查询任务 | 按 `(context, [version], id)` 查询已加载的 Task |
| `contextState(ctxKey)` | 查询上下文状态 | 返回 `ContextState` 四态（`PackageManager.h:75`） |

### 1.2 initialize() success-gated 幂等守卫

`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp` 第 61-68 行实现幂等守卫：

```cpp
Expected<void> Manager::initialize() {
    __stdc_impl_t;
    // 幂等防护：已初始化则直接返回错误
    if (impl.initialized) {
        return Error(Error::AlreadyInitialized,
                     "Manager::initialize() has already been called");
    }
    // ... 初始化逻辑 ...
    impl.initialized = true;   // 仅成功末尾置 true（Manager.cpp:256）
    return {};
}
```

**关键事实**：
- 成功后重调 → 返回 `Error::AlreadyInitialized`（**非静默 no-op**）
- 失败的 `initialize()` **允许重试**（`initialized` 仅成功末尾置 true）
- 测试验证：`tests/catch2/tst_init_constraints.cpp` 的 `ic_failedInit_allowsRetry_notBlocked`

这是约束 **L-2** 的实现基础。

### 1.3 addPackagePath 时序约束

- `addPackagePath` 本身**不检查** `initialized` 状态（见 `file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp` 第 552-575 行），运行时调用会静默注册路径。
- 但因 `initialize()` 不可重跑（L-2），运行时新增的路径**永不被处理**，停留在 Pending 状态直至进程结束。
- 这是约束 **L-1**（自定义 G2P 仅启动时加载）与 **L-3**（运行时新增声库需重启）的实现基础。
- 约束 **L-4**（官方先于私有初始化）由 `initialize()` Phase 顺序保证，**与 `addPackagePath` 调用顺序无关**。

## 2. PackageManager

`PackageManager` 负责扫描 G2P 包目录、解析模块元数据、解析依赖关系。`Manager` 公有继承自 `PackageManager`。

**头文件**：`file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h`

### 2.1 职责

- **包扫描**：遍历 `addPackagePath` 注册的目录，发现 G2P 包并加载模块元数据（`ModuleMetadata`）。
- **依赖解析**：基于 `DependencyResolver` 解析模块间依赖，生成可用的模块图。
- **上下文状态管理**：维护每个上下文的 `ContextState`，供宿主查询可用性。
- **错误收集**：采用 `collectError` 模式（决策 D7），单个**私有**上下文解析失败不中断整体流程；**默认**上下文失败则整体中止。

### 2.2 上下文状态机（ContextState）

`PackageManager.h:29-38` 定义了上下文的四种状态：

```cpp
enum class ContextState {
    Pending,       ///< 已注册但尚未初始化
    Ready,         ///< 初始化成功（至少一个包加载成功）
    Failed,        ///< 初始化失败（依赖缺失/环/Level 不兼容/driver 不可用）
    NotRegistered, ///< 未注册（不在 contexts() 枚举中）
};
```

> 注：`NotRegistered` 是**计算态**，非存储态。内部 Impl 仅存 3 态（`PackageManager_p.h:89-91` 的 `enum class ContextState { Pending, Ready, Failed }`），`contextState()` 在未命中时合成 `NotRegistered`（`PackageManager.cpp:690-714`）。

#### 状态语义

| 状态 | 语义 |
| --- | --- |
| `Pending` | 已注册但未初始化（`addPackagePath` 后、`initialize()` 前/中） |
| `Ready` | 初始化成功，上下文可用 |
| `Failed` | 初始化失败（依赖缺失/环/Level 不兼容/driver 不可用） |
| `NotRegistered` | 上下文未注册（未调用 `addPackagePath`，或歌手未安装） |

#### 状态转移图

```
                  addPackagePath()
  NotRegistered ─────────────────► Pending
                                          │
                            initialize()  │  Phase 1/2 扫描 + 依赖解析
                                 ┌────────┴────────┐
                                 ▼                 ▼
                               Ready            Failed
                            （可用）          （依赖失败）
                                                    │
                                            修复后重启 │
                                                    ▼
                                               Pending
```

- `NotRegistered → Pending`：宿主调用 `addPackagePath` 注册上下文路径。
- `Pending → Ready`：`initialize()` 扫描完成且依赖解析成功。
- `Pending → Failed`：`initialize()` 扫描完成但依赖解析失败。
- `Ready` / `Failed` 为终态（在当前进程生命周期内，受 L-2 幂等约束不可回退；修复需重启）。

### 2.3 R-8 版本约束

`PackageManager.cpp:582-587` 实现版本-上下文约束：

```cpp
if (context.empty() && !version.isEmpty())
    return Error(Error::ValidationError, "R-8: Default context cannot have a version");
if (!context.empty() && version.isEmpty())
    return Error(Error::ValidationError,
                 "Context '" + context + "': version cannot be empty ...");
```

- **默认上下文**（`context=""`）：version **必须为空**
- **私有上下文**（`context` 非空）：version **必须非空**（使用版本化重载时）

## 3. G2pRes 转换结果

`G2pRes` 是 `convert()` 的返回元素，承载转换结果与错误信息。

**头文件**：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`

### 3.1 核心 API

| 方法 | 行号 | 语义 |
| --- | --- | --- |
| `isOk()` | `LangCommon.h:58` | `errorType == NoError` 时返回 `true` |
| `isFailed()` | `LangCommon.h:62` | `errorType != NoError` 时返回 `true` |

### 3.2 字段

| 字段 | 说明 |
| --- | --- |
| `pronunciation` | 转换出的发音（主结果） |
| `candidates` | 候选发音列表 |
| `g2pId` | 实际命中的 G2P 模块 ID |
| `g2pContext` | 实际命中的上下文（D10：原 `context`） |
| `g2pContextVersion` | 实际命中的上下文版本（D10：原 `contextVersion`） |
| `g2pSource` | 来源标识：`"official"`（默认上下文）/ `"voicebank"`（私有上下文），D10 新增 |
| `errorType` | 错误类型（`NoError` 表示成功）；**D12**：`G2pErrorType` 为本仓库独家定义，调用方透传 |

> 宿主侧的回退策略（`ToOfficial` / `Never`）位于 `ds-editor-lite` 的 `G2pConvertRunner`，依据 `isFailed()` 判断是否触发回退。框架 `convert()` 本身无策略。详见 [03-host-integration-contract.md](03-host-integration-contract.md)。

## 4. 插件系统

LangCore 采用 **IID + 分类（category）** 机制组织插件：`addPluginPath(iid, path)` 按 IID 注册插件目录，`category(name)` 按分类名获取对象池。

### 4.1 插件 IID

| IID | 用途 | 实际目录示例 |
| --- | --- | --- |
| `org.openvpi.Driver` | 驱动类插件（如 ONNX 驱动） | `Drivers/` |
| `org.openvpi.Task` | 任务类插件（G2p / Tagger / Splitter 等） | `G2ps/`、`Taggers/`、`Splitters/` |

> ⚠️ `addPluginPath` 首参是 **IID 字符串**，不是 category 名。`tagger`/`splitter` 作为 Task 子类，通过同一 IID `org.openvpi.Task` 注册不同目录。

### 4.2 分类（category）

| 分类 | 用途 | 说明 |
| --- | --- | --- |
| `driver` | 驱动对象池（如 ONNX 驱动实例） | ONNX 驱动以裸名注册于此 |
| `g2p` | G2P 任务对象池 | ChainG2p 等实例 |
| `dict` | 字典任务对象池 | 字典查询实例 |

> 注：框架核心分类为 `driver`/`g2p`/`dict`。`TaggerRes` 等类型仍存在于 `LangCommon.h`，但标签/切分的前端逻辑已移至宿主侧。

### 4.3 DriverPlugin / TaskPlugin

- `DriverPlugin`：驱动类插件基类，提供底层推理能力（如 ONNX 推理）。
- `TaskPlugin`：任务类插件基类，G2P 等任务属此类。
- 插件通过 `addPluginPath(iid, path)` 注册的目录被发现并加载。

### 4.4 ONNX 驱动注册示例

ONNX 驱动作为 **全局基础设施**，以裸名 `g2pOnnxDriver` 注册到 `driver` 分类，**不参与上下文隔离**（决策 D5）：

```cpp
auto &driverCategory = *mgr->category("driver");
driverCategory.addObject("g2pOnnxDriver", onnxDriver);   // 裸名，不经过 formatFqid
```

> `addObject` 作用于 ObjectPool，裸名不经 `formatFqid()` 命名空间化，独立于 `(context, version)` 键空间。详见 [03-host-integration-contract.md](03-host-integration-contract.md) 的「ONNX 驱动注册模式」。

## 5. ModelStep 与 S5 修复

`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp` 第 52-67 行实现了 FQID 两级查找，修复了 S5 漏洞（决策 D4）：

```cpp
// 第一级：当前上下文按 FQID 查找
const auto fqid = LangCore::ContextUtils::formatFqid(ctxKey, m_onnxG2pId);
auto g2pObj = g2pCate->getFirstObject(fqid);
// 第二级：未命中且非默认上下文 → 回退默认上下文（裸 moduleId）
if (!g2pObj && !ctxKey.isDefault()) {
    g2pObj = g2pCate->getFirstObject(m_onnxG2pId);   // 裸 moduleId，非 ":g2pId"
}
```

- **第一级**：在当前上下文查找 `formatFqid(ctxKey, moduleId)`
- **第二级（回退）**：在默认上下文查找**裸 moduleId**（无冒号前缀），因为 `formatFqid` 对默认上下文也返回裸 moduleId

> 详细机制见 [02-context-isolation-mechanism.md](02-context-isolation-mechanism.md) 的「S5 漏洞修复」章节。

## 6. 能力清单速查

| 能力 | 入口 | 关键约束 |
| --- | --- | --- |
| 单例获取 | `Manager::instance()` | — |
| 插件路径注册 | `addPluginPath(iid, path)` | 按 IID；须在 `initialize()` 前调用 |
| 包路径注册 | `addPackagePath` | 须在 `initialize()` 前（否则不处理）；默认上下文 version 必空（R-8） |
| 初始化 | `initialize()` | success-gated 幂等（L-2）；成功后返回 Error |
| 初始化查询 | `initialized()` | 公共可查 |
| 批量转换 | `convert(inputs)` | 返回 `vector<G2pRes>`；框架无回退策略 |
| 分类对象获取 | `category(name)` | 用于注册 ONNX 驱动等 |
| 上下文状态查询 | `contextState(ctxKey)` | 四态：Pending/Ready/Failed/NotRegistered |
| 失败上下文列表 | `failedContexts()` | 排除默认上下文（`PackageManager.h:78`） |
| 结果判定 | `G2pRes::isOk()` / `isFailed()` | 基于 `errorType` |

## 7. 源码引用

- Manager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`
- Manager 实现：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`
- PackageManager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h`
- PackageManager 实现：`file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp`
- ContextKey / FQID：`file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h`
- 公共类型：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`
- ModelStep 实现：`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
- 幂等守卫测试：`file:///D:/projects/language-manager/tests/catch2/tst_init_constraints.cpp`
