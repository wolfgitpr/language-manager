# 03 · 宿主集成契约

本文档定义宿主工程（如 `ds-editor-lite`、`synthrt`）集成 LangCore 框架时必须遵守的契约，包括必须做、禁止做、加载约束、初始化顺序、ONNX 驱动注册模式、错误处理与回退策略。

> 2026-07-02 修订：修正 `addPluginPath` 参数（IID 非 category 名）、移除 tagger/splitter 分类、修正 `initialize()` 幂等行为（返回 Error 非 no-op）、修正 `addPackagePath` 后置行为（静默注册非 no-op）、修正 L-4 保证机制（Phase 顺序非注册顺序）。

## 1. 宿主必须做（Must Do）

### 1.1 按 IID 注册插件路径

```cpp
mgr->addPluginPath("org.openvpi.Driver", driversPath);   // 驱动插件
mgr->addPluginPath("org.openvpi.Task", g2psPath);        // G2P 任务插件
mgr->addPluginPath("org.openvpi.Task", taggersPath);     // Tagger 任务插件（可选）
mgr->addPluginPath("org.openvpi.Task", splittersPath);   // Splitter 任务插件（可选）
```

> ⚠️ `addPluginPath` 首参是 **IID 字符串**（`org.openvpi.Driver` / `org.openvpi.Task`），不是 category 名（如 `"driver"`/`"g2p"`）。Tagger/Splitter 通过同一 IID `org.openvpi.Task` 注册不同目录。

### 1.2 先注册官方默认上下文

```cpp
mgr->addPackagePath("", officialPackagesPath);   // 官方默认上下文，version 隐含为空
```

### 1.3 再注册声库私有上下文（逐声库）

```cpp
// 对每个含自定义 G2P 的声库语言：
mgr->addPackagePath(singerId, version, voicebankPackagePath);
```

> `registerAll()` **不是框架 API**。宿主需自行遍历已安装声库，对含自定义 G2P 的语言调用 `addPackagePath`。ds-editor-lite 的 `LanguagePackageRegistrar::registerAll()` 是此逻辑的宿主侧实现。

### 1.4 初始化 ONNX 驱动（裸名注册）

```cpp
// 1. 加载 ONNX 驱动插件
const auto onnxDriverPlugin = mgr->plugin<LangCore::DriverPlugin>("onnx");
auto expOnnxDriver = onnxDriverPlugin->create();
const auto onnxDriver = expOnnxDriver.take();

// 2. 初始化驱动（按需配置 ExecutionProvider 等）
onnxDriver->initialize(onnxArgs);

// 3. 以裸名注册到 driver 分类（不参与上下文隔离）
auto &driverCategory = *mgr->category("driver");
driverCategory.addObject("g2pOnnxDriver", onnxDriver);
```

### 1.5 调用 initialize() 完成初始化

```cpp
const auto initResult = mgr->initialize();
if (!initResult) {
    // 失败处理：initResult.error() 含错误信息
}
if (!mgr->initialized()) {
    // 验证失败
}
```

### 1.6 调用 convert() 执行转换

```cpp
std::vector<LangCore::G2pInput> inputs = /* 构造输入 */;
auto results = mgr->convert(inputs);
// 依据 G2pRes::isOk() / isFailed() 判断结果
```

## 2. 宿主禁止做（Must Not Do）

| 禁止行为 | 原因 | 后果 |
| --- | --- | --- |
| 在 `initialize()` 成功后再次调用 `initialize()` | success-gated 幂等（L-2） | 返回 `Error::AlreadyInitialized` |
| 期望运行时 `addPackagePath` 生效 | L-1 约束 | 路径静默注册但永不被处理，需重启 |
| 为默认上下文传非空 version | R-8 硬约束 | `addPackagePath` 返回 `Error::ValidationError` |
| 为私有上下文（版本化重载）传空 version | R-8 硬约束 | `addPackagePath` 返回 `Error::ValidationError` |
| 使用 `:g2pId` 作为默认上下文 FQID | `formatFqid` 对默认上下文返回裸 moduleId | 查找失败 |
| 期望 `convert()` 跨上下文回退 | C-6 不变式 | `convert()` 不跨上下文查找；回退由宿主 `G2pConvertRunner` 实现 |
| 在 `Manager` 单例之外创建 `Manager` 实例 | 单例模式 | 未定义行为 |
| 期望默认上下文失败后私有上下文可用 | Ord-1 | 默认上下文失败整体中止 |

> 注：`addPackagePath` 在 `initialize()` 后调用**不会**被拒绝（不返回 Error），而是静默注册但永不处理。宿主应在 `initialize()` 前完成所有 `addPackagePath` 调用。

## 3. 加载约束（L-1～L-4）

### L-1：自定义 G2P 仅在启动时加载

- **约束**：自定义 G2P 包仅在应用启动阶段加载，运行时禁止加载。
- **框架行为**：`addPackagePath` 不检查 `initialized`，但新路径因 `initialize()` 不可重跑而永不被处理。
- **宿主守卫**：ds-editor-lite 的 `LanguagePackageRegistrar::registerAll()` 在 `langMgr->initialized()` 为 true 时直接返回 0。

### L-2：initialize() success-gated 幂等

- **约束**：`initialize()` 成功后再次调用返回 `Error::AlreadyInitialized`；失败的 `initialize()` 允许重试。
- **实现**：`initialized` 标志仅在成功末尾置 true（`Manager.cpp:256`）。
- **测试**：`tests/catch2/tst_init_constraints.cpp` 的 `ic_failedInit_allowsRetry_notBlocked`。

### L-3：运行时安装新声音库需重启

- **约束**：运行时安装的新声音库（含自定义 G2P）需重启应用才能生效。
- **原因**：L-1 推论。

### L-4：官方上下文先于私有上下文初始化

- **约束**：默认上下文必须先于私有上下文**初始化**。
- **保证机制**：由 `initialize()` Phase 1（默认）/ Phase 2（私有）顺序硬编码保证（`Manager.cpp:74` 硬编码 `ContextKey("")`），**与 `addPackagePath` 调用顺序无关**。
- **影响**：宿主即使先注册私有上下文、后注册官方上下文，`initialize()` 仍保证官方先处理。

## 4. 初始化顺序

### 4.1 完整初始化序列

```
1. addPluginPath(iid, path) ×N
   │  按 IID 注册插件搜索路径
   │  IID: "org.openvpi.Driver" / "org.openvpi.Task"
   ▼
2. addPackagePath("", officialPath)
   │  注册官方默认上下文（version 隐含为空）
   ▼
3. addPackagePath(singerId, version, voicebankPath) ×N
   │  逐声库注册私有上下文（宿主侧编排，如 LanguagePackageRegistrar::registerAll）
   ▼
4. ONNX 驱动初始化 + 裸名注册
   │  category("driver")->addObject("g2pOnnxDriver", drv)
   ▼
5. initialize()
   │  success-gated 幂等（L-2）
   │  Phase 1: 默认上下文（失败则整体中止，Ord-1）
   │  Phase 2: 私有上下文（失败标记 Failed 并继续，collectError）
   │  Phase 3: Task 加载
   ▼
6. initialized() 验证
   ▼
7. convert(inputs) 执行 G2P 转换
```

### 4.2 ds-editor-lite 实际序列参考

ds-editor-lite 的 `LaunchLanguageEngineTask::runTask()`（`file:///D:/projects/ds-editor-lite/src/app/Controller/Tasks/LaunchLanguageEngineTask.cpp` 第 130-186 行）实现了上述序列，含 `waitForPackageModuleReady` 等待 PackageManager 扫描就绪的额外步骤。

### 4.3 降级行为

即使声库私有上下文注册全部失败（步骤 3 跳过），只要 `initialize()` 成功，官方 G2P（默认上下文）仍可用。

## 5. ONNX 驱动注册模式

ONNX 驱动是**全局基础设施**，以裸名注册，不参与上下文隔离（决策 D5）：

| 属性 | 值 |
| --- | --- |
| 注册分类 | `driver` |
| 注册名 | `"g2pOnnxDriver"`（裸名） |
| 注册方式 | `category("driver")->addObject("g2pOnnxDriver", drv)` |
| 上下文隔离 | ❌ 不参与 |
| 生命周期 | 应用级（全局单例） |
| 注册时机 | `initialize()` 之前 |

> 裸名注册意味着不经过 `formatFqid()` 命名空间化，独立于 `(context, version)` 键空间，所有上下文共享同一 ONNX 驱动实例。

## 6. 错误处理

### 6.1 initialize() 错误

| 错误源 | 错误码 | 处理 |
| --- | --- | --- |
| 重复调用（成功后） | `AlreadyInitialized` | 宿主应避免；检查 `initialized()` |
| 默认上下文失败 | Ord-1 | 整体中止，宿主应提示用户检查官方 G2P 包 |
| 私有上下文失败 | collectError 收集 | 不中止；可通过 `failedContexts()` 查询失败列表 |

### 6.2 addPackagePath 错误

| 错误源 | 错误码 | 处理 |
| --- | --- | --- |
| 默认上下文传 version | `ValidationError` (R-8) | 宿主应传空 version |
| 私有上下文传空 version | `ValidationError` (R-8) | 宿主应传声库声明版本 |
| context 名非法 | `ValidationError` | 宿主应校验 `singerId` 字符集 |
| 路径不存在/非目录 | `ValidationError` | 宿主应校验路径 |

### 6.3 convert() 错误

`convert()` 返回 `vector<G2pRes>`，每个 `G2pRes` 含 `errorType`：

| 字段 | 判定 | 宿主处理 |
| --- | --- | --- |
| `isOk()` | `errorType == NoError` | 使用 `pronunciation`/`candidates` |
| `isFailed()` | `errorType != NoError` | 依据场景选择回退策略 |

> 框架 `convert()` 本身无回退策略。回退由宿主侧 `G2pConvertRunner` 实现。

## 7. 回退策略（宿主侧，D11 统一 Never-only）

> **2026-07-02 决策（D11）**：回退策略统一为 `Never`-only，移除原 `ToOfficial` 策略与 `G2pFallbackPolicy` 枚举。

回退策略位于 `ds-editor-lite` 的 `G2pConvertRunner`，框架不参与：

| 场景 | 策略 | 说明 |
| --- | --- | --- |
| 推理（Inference） | `Never` | 私有上下文失败不回退；copy fallback（原词保留）+ 精准 `G2pErrorType` 上报 |
| FillLyric（填词） | `Never` | 私有上下文失败不回退；copy fallback（原词保留）+ 精准 `G2pErrorType` 上报 |

**D11 前**：FillLyric 链路使用 `ToOfficial`（私有上下文失败时用默认上下文参数再次调用 `convert()`）。
**D11 后**：所有链路统一 `Never`，宿主不再二次调用 `convert()`；框架 `convert()` 行为不变（本就无策略）。失败由 UI 显式提示用户手动调整。

> `G2pErrorType`（D12）由本仓库独家定义，宿主侧直接透传，不新增 caller 级错误枚举。

## 8. 源码引用

- Manager API：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`
- PluginFactory（addPluginPath）：`file:///D:/projects/language-manager/core/include/LangCore/Core/PluginFactory.h`
- PackageManager（addPackagePath）：`file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h`
- initialize 幂等守卫：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`（第 61-68 行）
- R-8 版本约束：`file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp`（第 582-587 行）
- ds-editor-lite 启动序列：`file:///D:/projects/ds-editor-lite/src/app/Controller/Tasks/LaunchLanguageEngineTask.cpp`（第 130-186 行）
- ds-editor-lite 声库注册：`file:///D:/projects/ds-editor-lite/src/app/Modules/Language/LanguagePackageRegistrar.cpp`（第 69-136 行）
- 约束测试：`file:///D:/projects/language-manager/tests/catch2/tst_init_constraints.cpp`
