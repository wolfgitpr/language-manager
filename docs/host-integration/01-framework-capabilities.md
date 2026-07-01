# 01 · 框架能力审计

本文档审计 LangCore 框架对外暴露的核心能力，作为宿主集成与测试设计的能力基线。所有引用均基于已核实的代码事实。

## 1. Manager 单例

`Manager` 是 LangCore 的核心单例，统一管理插件路径、包路径、初始化与转换入口。

**头文件**：`core/include/LangCore/Core/Manager.h`

### 1.1 核心 API

| 方法 | 签名要点 | 说明 |
| --- | --- | --- |
| `instance()` | 静态单例获取 | 返回 `Manager` 单例实例 |
| `addPluginPath(category, path)` | 添加插件搜索路径 | 按分类（driver / g2p / tagger / splitter）注册插件目录 |
| `addPackagePath(context, path)` | 添加 G2P 包路径 | `context=""` 表示官方默认上下文；**必须在 `initialize()` 之前调用** |
| `addPackagePath(context, version, path)` | 添加带版本的 G2P 包路径 | 支持版本化上下文 |
| `initialize()` | 完成初始化 | 幂等：已初始化后再次调用为 no-op |
| `initialized()` | 公共查询方法 | 返回是否已完成初始化（`Manager.h:27`） |
| `convert(inputs)` | 批量 G2P 转换 | 入参为 `vector<G2pInput>`，返回 `vector<G2pRes>` |
| `category(name)` | 获取分类对象 | 用于向分类注册对象（如 ONNX 驱动注册到 `driver` 分类） |

### 1.2 initialize() 幂等守卫

`core/lib/Core/Manager.cpp` 第 64–68 行实现了幂等守卫：若已初始化，则直接返回，不重复执行扫描与依赖解析逻辑。这是约束 **L-2** 的实现基础。

```
// 概要（详见 Manager.cpp:64-68）
if (initialized()) return;  // 幂等守卫
```

### 1.3 addPackagePath 时序约束

- `addPackagePath` **必须**在 `initialize()` 之前调用。
- 一旦 `initialize()` 完成，后续 `addPackagePath` 为 no-op（被忽略）。
- 这是约束 **L-4**（官方上下文先于私有上下文注册）与 **L-3**（运行时新增声库需重启）的实现基础。

## 2. PackageManager

`PackageManager` 负责扫描 G2P 包目录、解析模块元数据、解析依赖关系。

**头文件**：`core/include/LangCore/Core/PackageManager.h`

### 2.1 职责

- **包扫描**：遍历 `addPackagePath` 注册的目录，发现 G2P 包并加载模块元数据（`ModuleMetadata`）。
- **依赖解析**：基于 `DependencyResolver` 解析模块间依赖，生成可用的模块图。
- **上下文状态管理**：维护每个上下文的 `ContextState`，供宿主查询可用性。
- **错误收集**：采用 `collectError` 模式（决策 D7），单个包解析失败不会中断整体流程，错误被收集后继续。

### 2.2 上下文状态机（ContextState）

`PackageManager.h:33-38` 定义了上下文的四种状态：

```cpp
enum class ContextState { Pending, Ready, Failed, NotRegistered };
```

#### 状态语义

| 状态 | 语义 |
| --- | --- |
| `Pending` | 包正在扫描中，上下文尚未就绪 |
| `Ready` | 包已加载完成，依赖解析成功，上下文可用 |
| `Failed` | 依赖解析失败，上下文不可用 |
| `NotRegistered` | 上下文未注册（未调用 `addPackagePath`） |

#### 状态转移图

```
                  addPackagePath()
  NotRegistered ─────────────────► Pending
                                          │
                            initialize()  │  扫描 + 依赖解析
                                 ┌────────┴────────┐
                                 ▼                 ▼
                               Ready            Failed
                            （可用）          （依赖失败）
```

- `NotRegistered → Pending`：宿主调用 `addPackagePath` 注册上下文路径。
- `Pending → Ready`：`initialize()` 扫描完成且依赖解析成功。
- `Pending → Failed`：`initialize()` 扫描完成但依赖解析失败。
- `Ready` / `Failed` 为终态（在当前进程生命周期内，受 L-2 幂等约束不可回退）。

## 3. G2pRes 转换结果

`G2pRes` 是 `convert()` 的返回元素，承载转换结果与错误信息。

**头文件**：`core/include/LangCore/Base/LangCommon.h`

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
| `context` | 实际命中的上下文 |
| `contextVersion` | 实际命中的上下文版本 |
| `errorType` | 错误类型（`NoError` 表示成功） |

> 宿主侧的回退策略（`ToOfficial` / `Never`）依据 `isFailed()` 判断是否触发回退逻辑，详见 [03-host-integration-contract.md](03-host-integration-contract.md)。

## 4. 插件系统

LangCore 采用 **分类（category）机制** 组织插件，宿主通过 `Manager::category(name)` 获取分类对象后注册具体实例。

### 4.1 插件分类

| 分类 | 用途 |
| --- | --- |
| `driver` | 驱动插件（如 ONNX 驱动） |
| `g2p` | G2P 插件（如 ChainG2p） |
| `tagger` | 文本标签器（语言检测） |
| `splitter` | 文本切分器 |

### 4.2 DriverPlugin / TaskPlugin

- `DriverPlugin`：驱动类插件基类，提供底层推理能力（如 ONNX 推理）。
- `TaskPlugin`：任务类插件基类，G2P / Tagger / Splitter 等均属此类。
- 插件通过 `addPluginPath` 注册的目录被发现并加载。

### 4.3 ONNX 驱动注册示例

ONNX 驱动作为 **全局基础设施**，以裸名 `g2pOnnxDriver` 注册到 `driver` 分类，**不参与上下文隔离**（决策 D5）：

```cpp
auto &driverCategory = *mgr->category("driver");
driverCategory.addObject("g2pOnnxDriver", onnxDriver);
```

> 详见 [03-host-integration-contract.md](03-host-integration-contract.md) 的「ONNX 驱动注册模式」。

## 5. ModelStep 与 S5 修复

`plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp` 第 53–60 行实现了 FQID 两级查找，修复了 S5 漏洞（决策 D4）。

- **第一级**：在当前上下文查找 `context:g2pId`
- **第二级（回退）**：在默认上下文查找 `:g2pId`（空 context 前缀）

> 详细机制见 [02-context-isolation-mechanism.md](02-context-isolation-mechanism.md) 的「S5 漏洞修复」章节。

## 6. 能力清单速查

| 能力 | 入口 | 关键约束 |
| --- | --- | --- |
| 单例获取 | `Manager::instance()` | — |
| 插件路径注册 | `addPluginPath` | 须在 `initialize()` 前 |
| 包路径注册 | `addPackagePath` | 须在 `initialize()` 前；官方先于私有 |
| 初始化 | `initialize()` | 幂等（L-2） |
| 初始化查询 | `initialized()` | 公共可查 |
| 批量转换 | `convert(inputs)` | 返回 `vector<G2pRes>` |
| 分类对象获取 | `category(name)` | 用于注册 ONNX 驱动等 |
| 上下文状态查询 | `PackageManager` 的 `ContextState` | 四态：Pending/Ready/Failed/NotRegistered |
| 结果判定 | `G2pRes::isOk()` / `isFailed()` | 基于 `errorType` |

## 7. 源码引用

- Manager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`
- Manager 实现：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`
- PackageManager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h`
- 公共类型：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`
- ModelStep 实现：`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
