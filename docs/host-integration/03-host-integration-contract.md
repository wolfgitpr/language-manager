# 03 · 宿主集成契约

本文档定义 LangCore 宿主（`ds-editor-lite`、`synthrt`）集成的契约：必须履行的步骤、禁止行为、加载约束、初始化顺序、ONNX 驱动注册模式与错误处理。

## 1. 宿主必须做的事

### 1.1 添加插件路径

按分类添加插件搜索路径，覆盖 Drivers / G2ps / Taggers / Splitters：

```cpp
auto mgr = Manager::instance();
mgr->addPluginPath("driver",   driversPath);
mgr->addPluginPath("g2p",      g2psPath);
mgr->addPluginPath("tagger",   taggersPath);
mgr->addPluginPath("splitter", splittersPath);
```

### 1.2 添加包路径（先官方后私有）

- **官方默认上下文**：`context=""`，必须最先注册（约束 L-4）。
- **声库私有上下文**：`context=singerId`，在官方之后注册。

```cpp
// 官方默认上下文（必须先注册）
mgr->addPackagePath("", officialG2pPackagesPath);

// 声库私有上下文（在官方之后）
for (auto &singer : singers) {
    if (!singer.g2pPackagePath.empty()) {
        mgr->addPackagePath(singer.id, singer.g2pPackagePath);
    }
}
```

### 1.3 初始化 ONNX 驱动（裸名，不参与上下文隔离）

ONNX 驱动作为 **全局基础设施**，以裸名 `g2pOnnxDriver` 注册到 `driver` 分类（决策 D5），**不绑定到任何私有上下文**：

```cpp
auto &driverCategory = *mgr->category("driver");
driverCategory.addObject("g2pOnnxDriver", onnxDriver);
```

> 注意：`g2pOnnxDriver` 是裸名，不经过 `context:` 前缀化，对所有上下文全局可见。模型模块通过 ModelStep 两级查找引用底层驱动（见 [02-context-isolation-mechanism.md](02-context-isolation-mechanism.md) 第 5 节）。

### 1.4 调用 initialize()（一次性，幂等）

```cpp
mgr->initialize();  // 幂等，重复调用为 no-op（L-2）
```

### 1.5 调用 convert() 执行 G2P 转换

```cpp
auto results = mgr->convert(inputs);  // inputs: vector<G2pInput>
for (auto &r : results) {
    if (r.isOk()) { /* 使用 r.pronunciation */ }
    else         { /* 处理失败，按策略回退 */ }
}
```

## 2. 宿主禁止做的事

| 禁止行为 | 原因 |
| --- | --- |
| 在 `initialize()` 之后调用 `addPackagePath()` | 已初始化后注册被忽略（no-op），无法生效；需重启（L-3） |
| 多次调用 `initialize()` | 幂等守卫使其为 no-op，但不应依赖此行为；语义上只应调用一次（L-2） |
| 将 ONNX 驱动注册到私有上下文 | ONNX 驱动是全局基础设施，必须裸名注册（D5） |
| 运行时加载自定义 G2P | 自定义 G2P 仅启动时加载（L-1），运行时新增声库需重启（L-3） |
| 跨私有上下文引用模块 | 上下文隔离禁止 `SingerA` 解析到 `SingerB` 的模块 |
| 在 `initialize()` 完成前调用 `convert()` | `Pending` 状态下转换不可用 |

## 3. 加载约束（L-1～L-4）

| 约束 | 内容 | 实现依据 |
| --- | --- | --- |
| **L-1** | 自定义 G2P 仅在启动时加载 | 插件与包扫描发生在 `initialize()`，之后不再扫描 |
| **L-2** | `initialize()` 幂等，不可重复调用 | `Manager.cpp:64-68` 幂等守卫 |
| **L-3** | 运行时新增声库需重启 | `addPackagePath` 在 `initialize()` 后失效 |
| **L-4** | 官方上下文必须在私有上下文之前注册 | 默认上下文作为兜底来源，须先就绪 |

## 4. 初始化顺序

完整初始化顺序（参照 `ds-editor-lite` 的 `LaunchLanguageEngineTask` 模式）：

```
① addPluginPath(driver/g2p/tagger/splitter)
        │
        ▼
② addPackagePath("", officialPath)          ← 官方默认上下文（L-4 最先）
        │
        ▼
③ 为每个声库 addPackagePath(singerId, path) ← 私有上下文（L-4 之后）
        │
        ▼
④ initializeOnnxDriver → category("driver").addObject("g2pOnnxDriver", drv)  ← 裸名注册（D5）
        │
        ▼
⑤ mgr->initialize()                         ← 幂等初始化（L-2）
        │
        ▼
⑥ 可调用 mgr->convert(inputs)               ← G2P 转换
```

> 顺序违规的后果：
> - ③ 在 ② 之前：违反 L-4，兜底来源未就绪。
> - ②/③ 在 ⑤ 之后：注册被忽略（no-op），上下文未生效。
> - ④ 在 ⑤ 之后：驱动注册晚于初始化，依赖该驱动的模块解析失败。

## 5. ONNX 驱动注册模式

### 5.1 裸名注册

```cpp
auto &driverCategory = *mgr->category("driver");
driverCategory.addObject("g2pOnnxDriver", onnxDriver);
```

- **裸名**：`g2pOnnxDriver`，不带任何 `context:` 前缀。
- **位置**：`driver` 分类，全局可见。
- **隔离性**：不参与上下文隔离，所有上下文共享同一驱动实例。

### 5.2 为什么不上下文隔离

ONNX 驱动是底层推理引擎，属于进程级共享资源。若每个私有上下文各注册一份：

- 重复加载模型，内存浪费。
- 驱动实例与 G2P 模块的多对多关系复杂化。
- 与「模型模块通过 ModelStep 两级查找共享」的设计冲突。

因此驱动层全局化（D5），模型模块层通过两级查找实现「私有优先 + 默认兜底」（D4），两者分层配合。

## 6. 错误处理：collectError 模式

### 6.1 单包失败不阻塞

`PackageManager` 在扫描与依赖解析时采用 `collectError` 模式（决策 D7）：

- 单个 G2P 包解析失败 → 收集错误，**继续解析其余包**。
- 受影响的上下文置为 `Failed`，其他上下文正常进入 `Ready`。

### 6.2 宿主侧处理

- 宿主通过 `ContextState` 查询各上下文可用性。
- `convert()` 返回的 `G2pRes` 通过 `isOk()` / `isFailed()` 判定单条结果成败。
- 失败结果按回退策略处理（见第 7 节）。

## 7. 回退策略（宿主侧，G2pConvertRunner）

> `G2pConvertRunner` 位于 `ds-editor-lite`，不在 LangCore 内；此处仅描述契约以指导测试设计。

### 7.1 两条调用路径

| 路径 | 触发场景 | 回退策略 |
| --- | --- | --- |
| FillLyric（填词） | 用户填词 | `ToOfficial`：私有失败 → 回退官方上下文重试 |
| Inference（推理） | `GetPronunciationTask` | `Never`：仅主转换，失败时复制 fallback（lyric），不回退官方 |

### 7.2 ToOfficial 流程

1. 主转换：`mgr->convert(inputs)`，使用声库私有上下文。
2. 检测失败：若 `result.isFailed()` 且 `context` 非空（私有上下文）。
3. 回退转换：将 `context` 改为 `""`（官方），再次 `mgr->convert(inputs)`。
4. 结果映射到 `G2pResult`。

### 7.3 Never 流程

1. 主转换：`mgr->convert(inputs)`，使用声库私有上下文。
2. 检测失败：若 `result.isFailed()`。
3. **不回退官方**，直接复制 fallback（如原始 lyric）。
4. 结果映射到 pronunciation 字符串。

### 7.4 路由两级决策（G2pRouteResolver）

在调用 `convert` 之前，`G2pRouteResolver::resolve(singerInfo, language)` 进行两级路由：

- **第一级（声库上下文）**：若声库 `g2pPackagePaths` 非空 → `context = singerId`，`source = voicebank`。
- **第二级（官方上下文）**：若 `g2pPackagePaths` 为空 → `context = ""`，`source = official`。
- 路由无效条件：`resolutionState` 为 `Pending`/`Missing`、语言未找到、`g2pId` 为空/未知。

> 详细测试设计见 [04-test-design.md](04-test-design.md) 测试领域 2。

## 8. 宿主集成检查清单

集成完成前，请逐项核对：

- [ ] 已按分类添加所有插件路径（Drivers / G2ps / Taggers / Splitters）
- [ ] 官方默认上下文（`context=""`）已最先注册（L-4）
- [ ] 所有声库私有上下文已在官方之后注册
- [ ] ONNX 驱动以裸名 `g2pOnnxDriver` 注册到 `driver` 分类（D5）
- [ ] `initialize()` 仅调用一次（L-2）
- [ ] `convert()` 在 `initialize()` 完成后调用
- [ ] 失败结果按 `ToOfficial` / `Never` 策略处理
- [ ] 未在运行时加载自定义 G2P（L-1、L-3）

## 9. 源码引用

- Manager 头文件：`file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h`
- Manager 实现（幂等守卫）：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`
- 公共类型（G2pRes / G2pInput）：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`
- 端到端测试（宿主流程参考）：`file:///D:/projects/language-manager/tests/tst_langCore/main.cpp`
