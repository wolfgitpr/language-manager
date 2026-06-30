# Language Manager 架构概览

**版本**：2.0  
**日期**：2026-04-27  
**适用范围**：核心框架架构，不涉及具体插件实现细节（插件实现参见各插件目录及 Plugin-Development-Guide.md）

---

## 目录

1. [分层架构](#1-分层架构)
2. [继承层次](#2-继承层次)
3. [模块目录结构](#3-模块目录结构)
4. [初始化流程](#4-初始化流程)
5. [运行时调用流程](#5-运行时调用流程)
6. [依赖解析系统](#6-依赖解析系统)
7. [插件系统](#7-插件系统)
8. [多版本任务支持](#8-多版本任务支持)

---

## 1. 分层架构

框架采用六层结构，自顶向下职责递减，每层只依赖其下层。

```
┌─────────────────────────────────────────────────────┐
│  应用层    Manager                                   │
│           单例入口，提供 convert() / task() 高层 API  │
├─────────────────────────────────────────────────────┤
│  管理层    PackageManager                            │
│           包发现、依赖解析、模块元数据管理              │
├─────────────────────────────────────────────────────┤
│  工厂层    PluginFactory                             │
│           DLL 扫描与懒加载、插件实例化                 │
├─────────────────────────────────────────────────────┤
│  插件层    Plugin --> TaskPlugin / DriverPlugin       │
│           DLL 导出的插件单例，负责创建 Task            │
├─────────────────────────────────────────────────────┤
│  任务层    Task / SessionTask                        │
│           具体处理逻辑：G2p 转换、字典查询、模型推理    │
├─────────────────────────────────────────────────────┤
│  支持层    Expected<T>, Error, ConfigAccessor,       │
│           Logging, JSON, Tensor, ContextKey,          │
│           ContextUtils                                │
│           基础设施，供所有层使用                       │
└─────────────────────────────────────────────────────┘
```

**层间关系**：

- 应用层 → 管理层 → 工厂层：通过 public 继承形成链式关系（`Manager : PackageManager : PluginFactory`）
- 工厂层 → 插件层：DLL 动态加载，调用 `langCore_plugin_instance()` 获取 Plugin 单例
- 插件层 → 任务层：`TaskPlugin::createTask()` 或 `DriverPlugin::create()` 创建实例
- 支持层：横切关注点，所有层均可直接使用

---

## 2. 继承层次

### 2.1 NamedObject 体系

`NamedObject` 是带名称的基础对象，用于对象池管理。

```
NamedObject
 ├── ObjectPool                       容器基类
 │    └── ModuleCategory              同类模块容器（g2p, dict, driver）
 ├── TaskInfoBase                     任务数据基类
 │    ├── TaskInput                   任务输入
 │    │    ├── G2pInputV1             G2p 输入（词列表）
 │    │    ├── DictInputV1            字典查询输入
 │    │    └── SessionStartInput      推理会话输入（张量）
 │    ├── TaskResult                  任务输出
 │    │    ├── G2pResultV1            G2p 结果
 │    │    ├── DictResV1              字典查询结果
 │    │    └── SessionResult          推理会话结果（张量）
 │    └── TaskInitArgs                任务初始化参数
 │         ├── DriverInitArgs         驱动初始化参数
 │         └── SessionOpenArgs        会话打开参数
 ├── Task                             处理逻辑基类
 │    └── SessionTask                 AI 模型推理任务（带会话管理）
 └── SessionFactory                   推理会话工厂
```

### 2.2 Plugin 体系

Plugin 是独立基类，不继承 NamedObject。

```
Plugin                                插件基类（DLL 导出单例）
 ├── TaskPlugin                       核心插件（G2p, Dict），受 Level 检查约束
 └── DriverPlugin                     工具插件（OnnxDriver），无 Level 检查
```

### 2.3 管理链

```
PluginFactory                         DLL 扫描/加载
 └── PackageManager                   包管理 + 依赖解析
      └── Manager                     单例顶层入口
```

三者通过 public 继承串联。这意味着 `Manager` 实例同时拥有包管理和插件工厂的全部能力。

### 2.4 Package 与 ModuleSpec

```
Package                               插件包（id, version, vendor, modules）
ScopedPackageRef                      RAII 包引用（生命周期管理）

ModuleSpec                            模块元数据（id, category, className, apiLevel, config...）
 ├── DriverSpec                       驱动模块描述
 ├── G2pSpec                          G2p 模块描述
 └── DictSpec                         字典模块描述
```

### 2.5 错误处理

```
Error                                 框架层错误（11 种错误码 + Context + suggestion）
Expected<T>                           tagged union: T | Error，替代异常的错误传播机制
Expected<void>                        无返回值特化
```

### 2.6 张量接口

```
ITensor                               张量抽象接口
 └── Tensor                           CPU 实现（底层使用 AlignedAllocator）
```

### 2.7 Context 支持

```
ContextKey                            context 名称 + 版本复合键
ContextUtils                          FQID 解析/格式化, context 名称/moduleId 校验
```

### 2.8 多版本任务

```
VersionedTaskImplBase                 版本化实现接口
VersionedTaskManager                  持有 impl 实例，委托调用
```

---

## 3. 模块目录结构

```
language-manager/
│
├── core/
│   ├── include/LangCore/              公共头文件（31 个）
│   │   ├── LangCoreGlobal.h           导出宏定义
│   │   ├── Base/                      基础对象
│   │   │   ├── NamedObject.h          带名称基类
│   │   │   ├── ObjectPool.h           对象池
│   │   │   ├── LangCommon.h           通用数据结构（G2pInput, G2pRes, TaggerRes...）
│   │   │   └── AlignedAllocator.h     内存对齐分配器
│   │   ├── Support/                   基础设施
│   │   │   ├── Error.h                错误类型
│   │   │   ├── Expected.h             Expected<T> 错误包装
│   │   │   ├── ConfigAccessor.h       配置读取
│   │   │   ├── Logging.h              日志系统
│   │   │   ├── JSON.h                 JSON 工具
│   │   │   ├── Tensor.h               张量接口
│   │   │   ├── PhonemeDict.h          音素字典
│   │   │   ├── DisplayText.h          本地化显示文本
│   │   │   └── ContextUtils.h         ContextKey, FQID 解析/格式化, context 校验
│   │   ├── Core/                      核心管理
│   │   │   ├── Manager.h              顶层单例
│   │   │   ├── PackageManager.h       包管理器
│   │   │   ├── PluginFactory.h        插件工厂
│   │   │   ├── Plugin.h               插件基类
│   │   │   └── ManagerLogger.h        内置日志分类
│   │   ├── Task/                      任务系统
│   │   │   ├── Task.h                 任务基类
│   │   │   ├── SessionTask.h          会话任务
│   │   │   ├── TaskPlugin.h           TaskPlugin + DriverPlugin
│   │   │   ├── TaskFactory.h          任务工厂
│   │   │   ├── G2pTask.h              G2p 版本化 I/O 类型
│   │   │   ├── DictTask.h             Dict 版本化 I/O 类型
│   │   │   ├── VersionedTaskManager.h 多版本任务管理
│   │   │   └── VersionedTaskImplBase.h 版本化实现接口
│   │   ├── Module/                    模块系统
│   │   │   ├── Module.h               ModuleSpec, ModuleCategory, ModuleLocator
│   │   │   ├── ModuleCategories.h     预定义类别宏（g2p, dict, driver）
│   │   │   └── Dependency/            依赖解析子系统
│   │   │       ├── DependencyGraph.h          有向依赖图 + 拓扑排序
│   │   │       ├── DependencyResolver.h       依赖迭代解析
│   │   │       ├── LevelCompatibilityChecker.h Level 兼容校验
│   │   │       └── VersionUtils.h             版本号解析与范围匹配
│   │   └── Package/
│   │       └── Package.h              Package + ScopedPackageRef
│   │
│   └── lib/                           上述头文件的实现（.cpp）
│
├── plugins/                           动态加载插件
│   ├── G2ps/
│   │   ├── MandarinG2p/               普通话 G2p（cpp-pinyin）
│   │   ├── CantoneseG2p/             粤语 G2p（cpp-pinyin jyutping 模式）
│   │   ├── LstmG2p/                  LSTM 模型 G2p（ONNX，支持 V1/V2）
│   │   └── ChainG2p/                 责任链 G2p 框架
│   ├── Dicts/
│   │   └── DsDict/                   字典查询插件
│   ├── Drivers/
│   │   └── OnnxDriver/               ONNX Runtime 推理驱动
│   └── Utils/
│       ├── Common/                    公共工具
│       ├── InferUtil/                 推理辅助
│       └── OnnxUtil/                  ONNX 辅助
│
├── res/G2pPackages/                   语言包资源
│   ├── Phonetic-Suite-Cmn/           普通话
│   ├── Phonetic-Suite-Eng/           英语
│   ├── Phonetic-Suite-Jpn/           日语
│   ├── Phonetic-Suite-Yue/           粤语
│   ├── Phonetic-Suite-Num/           数字
│   ├── Phonetic-Suite-Punc/          标点
│   └── Phonetic-Suite-Unknown/       未知语言
│
├── tests/
│   ├── catch2/                       Catch2 L1/L2 测试（单一可执行目标 LangMgrTests，无需插件/运行时）
│   ├── common/                       Catch2 v2.13.10 单头文件 (catch.hpp)
│   └── tst_langCore/                 L4 集成测试（含本地 Splitter/Tagger 实现）
│       └── configs/                  测试用简化配置
│
└── docs/                              设计文档
```

---

## 4. 初始化流程

系统启动时，`Manager::initialize()` 触发完整的加载链，返回 `Expected<void>`。初始化分两阶段执行：Phase 1 处理默认 context（""），Phase 2 处理所有非默认 ContextKey。

```
Manager::initialize()  --> Expected<void>
 │
 ├── Phase 1: 默认 context ("")
 │    │
 │    ├── 1.1 扫描包目录
 │    │    遍历 res/G2pPackages/ 下所有子目录
 │    │    解析每个目录中的 package.json
 │    │
 │    ├── 1.2 collectModuleMetadata()
 │    │    从所有包的 modules 字段收集模块元数据
 │    │    按类别（g2p / dict / driver）分类注册
 │    │    读取各模块 config.json 中的 $version 和 level
 │    │
 │    ├── 1.3 去重（isSameMainModule / selectBestModules）
 │    │
 │    ├── 1.4 DependencyResolver::resolveAllDependencies()
 │    │    │
 │    │    ├── VersionResolver：
 │    │    │    packageId + moduleId 定位候选
 │    │    │    --> level 精确匹配过滤
 │    │    │    --> version 范围过滤
 │    │    │    --> 选取最高版本
 │    │    │
 │    │    └── 迭代解析（最多 2N 轮），检测缺失依赖
 │    │
 │    ├── 1.5 LevelCompatibilityChecker::checkCorePlugin()
 │    │    校验每个核心插件的 Level 满足: M-1 <= P <= M
 │    │    不兼容的插件被拒绝加载并输出详细错误日志
 │    │
 │    ├── 1.6 DependencyGraph::buildGraph()
 │    │    构建有向依赖图
 │    │    Kahn 拓扑排序（副产物：循环依赖检测）
 │    │    输出 PackageInitializationPlan（加载顺序）
 │    │
 │    ├── 1.7 loadPackagesInOrder()
 │    │    按拓扑顺序逐个加载
 │    │    │
 │    │    ├── PluginFactory::plugin(key)
 │    │    │    按 className 查找已注册的 DLL
 │    │    │    懒加载 DLL --> 调用 langCore_plugin_instance()
 │    │    │    获取 Plugin 单例
 │    │    │
 │    │    └── createModuleTask(spec)
 │    │         TaskPlugin::createTask(spec) 创建 Task 实例
 │    │         Task::initialize() 加载配置、初始化内部状态
 │    │
 │    └── 默认 context 失败 --> 整个 initialize() 返回 Error
 │
 ├── Phase 2: 所有非默认 ContextKey
 │    │
 │    ├── 对每个非默认 ContextKey 执行相同流程：
 │    │    collectModuleMetadata → 去重 → DependencyResolver
 │    │    （依赖解析失败时可回退到默认 context 的模块）
 │    │    → LevelChecker → DependencyGraph
 │    │    → loadPackagesInOrder → createModuleTask
 │    │
 │    └── 某个 context 失败 --> 该 context 标记为 Failed，其余继续
 │
 └── 所有 Phase 完成后返回 Expected<void>
```

**要点**：

- 整个流程是同步、单线程的
- 插件 DLL 采用懒加载：只有被某个模块引用时才实际载入
- 加载完成后，Plugin 和 Task 常驻内存，不做卸载
- 默认 context 失败会中断整个初始化；非默认 context 失败只影响该 context

---

## 5. 运行时调用流程

初始化完成后，外部通过 `Manager::convert()` 发起 G2p 转换请求。

```
应用代码                     Manager                      Task
  │                           │                            │
  │  convert(vector<G2pInput>)│                            │
  │ ─────────────────────────>│                            │
  │                           │                            │
  │                           │  按 (context, contextVersion,│
  │                           │   g2pId) 相邻分组           │
  │                           │                            │
  │                           │  Task 查找：               │
  │                           │  精确 ContextKey 匹配      │
  │                           │  → 回退到无版本 ContextKey  │
  │                           │  → 失败                    │
  │                           │                            │
  │                           │  构造 G2pInputV1            │
  │                           │  start(G2pInputV1)         │
  │                           │ ──────────────────────────>│
  │                           │                            │
  │                           │         G2pResultV1        │
  │                           │ <──────────────────────────│
  │                           │                            │
  │                           │  提取 G2pRes 列表           │
  │                           │  聚合所有结果               │
  │                           │                            │
  │  vector<G2pRes>           │                            │
  │ <─────────────────────────│                            │
```

**数据变换**：

```
G2pInput { lyric, g2pId, context, contextVersion }
    --> 按 (context, contextVersion, g2pId) 相邻分组
    --> G2pInputV1 { vector<string> g2pInput }
    --> Task::start()
    --> G2pResultV1 { vector<G2pRes>, errorMessage }
    --> 聚合回 vector<G2pRes>
```

**运行时 context 查找规则**：对每个分组，先按精确 ContextKey（context + contextVersion）查找已加载的 Task。若未找到，回退到同名但无版本的 ContextKey。不会跨 context 回退（跨 context 回退仅在依赖解析阶段发生）。

Task 内部处理因插件而异。字典型插件（MandarinG2p）直接查表，模型型插件（LstmG2p）通过 SessionTask 调用 OnnxDriver 进行推理，ChainG2p 则按责任链逐步处理。具体插件实现不在本文档范围内。

---

## 6. 依赖解析系统

依赖解析是初始化流程中最复杂的环节，由四个组件协作完成。

### 6.1 DependencyRequirement

模块间依赖在 `package.json` 中声明：

```json
{
  "packageId": "eng-official",
  "moduleId": "g2p-lstm-eng",
  "level": 2,
  "version": ">=1.0"
}
```

四个字段含义：`packageId` + `moduleId` 定位候选模块，`level` 做精确匹配（-1 表示跟随请求方），`version` 做范围过滤。

### 6.2 VersionResolver

负责从候选模块池中选出最佳匹配：

```
全部已注册模块
    |
    v
按 packageId + moduleId 过滤
    |
    v
按 level 精确匹配（-1 时跟随请求方 Level）
    |
    v
按 version 范围过滤（支持 >=, ~, *, 连字符范围等语法）
    |
    v
选取满足条件的最高版本
```

### 6.3 DependencyGraph

构建有向依赖图后执行 Kahn 拓扑排序：

```
输入：模块集合 + 依赖关系

1. 计算每个节点的入度
2. 将入度为 0 的节点加入队列
3. 循环：取出队首节点，减少其邻居入度
   入度变为 0 的邻居入队
4. 排序完成后，未被访问的节点即为循环依赖成员

输出：PackageInitializationPlan（拓扑有序的加载顺序）
```

不使用 Tarjan SCC 算法。Kahn 排序的副产物天然能检测出环。

### 6.4 LevelCompatibilityChecker

校验核心插件（TaskPlugin）与管理器间的 Level 兼容性：

```
Manager Level = M
Plugin Level  = P

兼容条件: M - 1 <= P <= M

示例（M = 3）:
  P = 2  -->  OK  (3 - 1 <= 2 ... 不对，2 <= 3 且 2 >= 2，OK)
  P = 3  -->  OK
  P = 4  -->  拒绝（超出上限）
  P = 1  -->  拒绝（低于下限）
```

工具插件（DriverPlugin）不受此规则约束，其兼容性通过依赖声明中的 level + version 自行校验。

---

## 7. 插件系统

### 7.1 插件类型

| 类型 | 基类 | 使用 Core 结构体 | Level 检查 | 用途 |
|------|------|:-:|:-:|------|
| 核心插件 | `TaskPlugin` | 是 | 是 | G2p 转换、字典查询 |
| 工具插件 | `DriverPlugin` | 否 | 否 | 推理驱动等辅助功能 |

判断规则：使用 `TaskInput`/`TaskResult` 或继承 `Task` 的都是核心插件。

### 7.2 导出宏

插件通过 C 函数 `langCore_plugin_instance()` 向框架暴露单例。框架提供简化宏，一行搞定类定义和导出：

```cpp
// 核心插件：定义 PluginClass 并导出，绑定 TaskClass
LANGCORE_DEFINE_TASK_PLUGIN(MandarinG2pPlugin, MandarinG2pTask,
                            "g2p.template.MandarinG2pInference", 1)

// 工具插件：定义 PluginClass 并导出，绑定 SessionFactory
LANGCORE_DEFINE_DRIVER_PLUGIN(OnnxDriverPlugin, OnnxSessionFactory,
                              "driver.onnx", 1)
```

宏内部调用 `LANGCORE_EXPORT_PLUGIN`，生成 `extern "C"` 导出函数。

### 7.3 DLL 加载机制

`PluginFactory` 管理所有插件 DLL 的生命周期：

```
1. scanPlugins()
   扫描插件目录，读取每个 DLL 旁的 plugin.json 描述文件
   记录 key --> DLL 路径 的映射

2. plugin(key)  [懒加载]
   首次调用时：
     加载 DLL（LoadLibrary / dlopen）
     查找 langCore_plugin_instance 符号
     调用获取 Plugin* 单例
     缓存并返回
   后续调用：
     直接返回缓存的 Plugin*
```

### 7.4 plugin.json 描述文件

每个插件 DLL 旁放置一个 `plugin.json`，声明插件元信息：

```json
{
  "className": "g2p.template.MandarinG2pInference",
  "iid": "org.openvpi.Task"
}
```

`className` 与宏中的 key 对应，`iid` 区分 TaskPlugin 和 DriverPlugin。

### 7.5 模块类别

系统预定义三种模块类别，通过宏注册：

```cpp
// 头文件声明
LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")
LANGCORE_DECLARE_MODULE_CATEGORY(Dict, "dict")
LANGCORE_DECLARE_MODULE_CATEGORY(Driver, "driver")

// 实现文件定义
LANGCORE_DEFINE_MODULE_CATEGORY(G2p, "g2p")
// ...
```

每个 `ModuleCategory` 是一个 `ObjectPool`，持有该类别下所有已加载的 Task 实例。

---

## 8. 多版本任务支持

当 API 结构演进（Level 递增）时，同一插件可能需要同时支持多个 Level 的输入/输出格式。框架通过 `VersionedTaskManager` 实现版本分派。

### 8.1 核心组件

```
VersionedTaskImplBase           纯虚接口，声明 initialize/start/getConfig
    |
    ├── V1::TaskImpl            Level 1 的实现
    └── V2::TaskImpl            Level 2 的实现

VersionedTaskManager            持有一个 VersionedTaskImplBase 实例
                                将 Task 的虚函数调用委托给 impl
```

### 8.2 单版本插件

只支持一个 Level 时，用 `TASK_IMPLEMENT` 宏一行生成全部委托代码：

```cpp
// 自动生成构造函数 + initialize/start/getConfig 委托
TASK_IMPLEMENT(MandarinG2pTask, MandarinG2pTaskImpl)
```

### 8.3 多版本插件

支持多个 Level 时，手写构造函数做 switch 分派，再用 `TASK_IMPLEMENT_METHODS` 生成剩余委托：

```cpp
LstmG2pTask::LstmG2pTask(const ModuleSpec *spec)
    : Task(spec), _manager(spec)
{
    switch (spec->apiLevel()) {
        case 2:
            _manager.setImpl(std::make_unique<V2::LstmG2pTaskImpl>(spec));
            break;
        default:
            _manager.setImpl(std::make_unique<V1::LstmG2pTaskImpl>(spec));
            break;
    }
}

// 生成 initialize(), start(), getConfig() 的委托代码
TASK_IMPLEMENT_METHODS(LstmG2pTask)
```

### 8.4 版本选择时机

版本选择发生在 Task 构造时（即 `TaskPlugin::createTask(spec)` 期间）。`spec->apiLevel()` 来自模块 config.json 中的 `level` 字段。一旦构造完成，impl 不再切换。

---

## 参考文档

- **PRD-v2.0.md** ：完整需求规格，包含错误处理、配置管理、测试等详细规范
- **Plugin-Development-Guide.md**：插件开发指南
- **ChainG2p-Design-Document.md**：ChainG2p 责任链设计
- **Test-Design-Document.md**：测试设计
