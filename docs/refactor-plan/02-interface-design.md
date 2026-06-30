# 02 — 接口与修复设计

> **日期**: 2026-07-01
> **原则**: 接口稳定（[ARCH-02](../decisions/human-decisions.md)）、不过度设计、抛弃技术债、修复 S5 漏洞
> **整合自**: 原 `lang-framework-plan/02-interface-design.md` + synthrt O-1/O-2/O-5 建议

---

## 1. 稳定公共 API 面（承诺稳定）

### 1.1 白名单（宿主侧仅使用以下 API）

| API | 签名 | 稳定性 |
|-----|------|--------|
| 单例入口 | `Manager *Manager::instance()` | ✅ 稳定 |
| 官方注册 | `Expected<void> addPackagePath(const std::string &context, const std::filesystem::path &path)` | ✅ 稳定 |
| 声库注册 | `Expected<void> addPackagePath(const std::string &context, const stdc::VersionNumber &version, const std::filesystem::path &path)` | ✅ 稳定 |
| 初始化 | `Expected<void> initialize()` | ✅ 稳定（§6 增加幂等防护，错误码 additive） |
| 转换 | `std::vector<G2pRes> convert(const std::vector<G2pInput> &input)` | ✅ 稳定 |
| task 查找 | `Expected<NO<Task>> task(category, context, id)` / `task(category, context, version, id)` | ✅ 稳定 |
| tasks 列举 | `Expected<std::vector<NO<Task>>> tasks(category, context)` / `tasks(category, context, version)` | ✅ 稳定 |
| context 枚举 | `std::vector<std::string> contexts()` / `std::vector<ContextKey> contextKeys()` | ✅ 稳定 |
| 数据结构 | `G2pInput(lyric, g2pId, context, contextVersion)` / `G2pRes`（8 参构造） | ✅ 稳定 |
| context 校验 | `ContextUtils::validateContextName` | ✅ 稳定 |
| FQID 工具 | `ContextUtils::formatFqid / parseFqid` | ✅ 稳定 |

### 1.2 新增 API（additive，非破坏性）

| 新增 API | 签名 | 用途 | 见 § |
|---------|------|------|------|
| ModuleSpec context | `ContextKey ModuleSpec::contextKey() const` | 暴露模块所属 context | §3 |
| context 状态查询 | `ContextState contextState(const ContextKey &) const` | 查询单个 context 状态 | §4 |
| 失败 context 列表 | `std::vector<ContextKey> failedContexts() const` | 列出所有 Failed context | §4 |
| G2pRes 成功判定 | `bool G2pRes::isOk() const` | `errorType == NoError`（含合法原词保留） | §5 |
| G2pRes 失败判定 | `bool G2pRes::isFailed() const` | `errorType != NoError`，等价 `!isOk()` | §5 |
| initialize 幂等防护 | `initialize()` 二次调用返回 `Error{AlreadyInitialized}` | 防止重跑全流程 | §6 |

**ARCH-02 合规**：以上均为 additive（新增方法/函数/错误码），不修改现有签名，不需递增 Level。

### 1.3 禁止使用（已 deprecated 或将私有化）

| API | 处理 |
|-----|------|
| `PackageManager::checkDependencies()` | 当前 Level（v3.x）追加 `[[deprecated]]` 编译期警告；私有化推迟到下一 Level（v4.x）并递增 Level（§2） |
| `PackageManager::getPackageInitializationOrder()` | 同上 |
| `PackageManager::loadPackagesInOrder()` | 同上 |
| `PackageManager::open(path)` 单独加载 | 保留 public 但文档说明"不解析传递依赖" |
| `G2pRes` 7 参数 legacy 构造 | 已 `[[deprecated]]`，保留 |
| `LANGPLUGINS_ENABLE_STATIC_PLUGINS` 选项 | 文档标注"暂不可用" |

---

## 2. 私有化 deprecated 扁平化方法（AD-F1 修复）

> **v3 对齐修订（消解内部不一致）**：以 [05 §ARCH-02](05-design-principles-check.md) 消解方案为准。当前 Level（v3.x）**仅追加 `[[deprecated]]` 编译期警告**，私有化推迟到下一 Level（v4.x）并递增 Level。原方案"直接 private 化"过于激进，违反 ARCH-02"破坏性变更须递增 Level"。

### 2.1 现状

[PackageManager.h:34-39, 62-64](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) 中三个方法为 `public`（通过继承暴露给宿主）：

```cpp
public:
    /// @deprecated 跨 context 扁平化，与隔离设计冲突。使用 Manager::initialize() 替代。
    bool checkDependencies();
    std::vector<PackageInitializationPlan> getPackageInitializationOrder();
    bool loadPackagesInOrder();
```

### 2.2 修复方案（分两 Level 推进）

**当前 Level（v3.x）**：保留 public 可见性，追加 `[[deprecated]]` 编译期警告：

```cpp
public:
    /// @deprecated 跨 context 扁平化，与隔离设计冲突。使用 Manager::initialize() 替代。
    /// v3.x：保留 public + 编译期警告；v4.x：私有化并递增 Level。
    [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
    bool checkDependencies();
    [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
    std::vector<PackageInitializationPlan> getPackageInitializationOrder();
    [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
    bool loadPackagesInOrder();
```

**下一 Level（v4.x，未来版本）**：将三个方法从 `public` 区移到 `private` 区（保留 `@deprecated` 注释），并递增 Level：

```cpp
private:
    /// @deprecated 跨 context 扁平化，与隔离设计冲突。使用 Manager::initialize() 替代。
    /// 已私有化：宿主不应调用，框架内部 Manager::initialize() 也不调用（使用两阶段初始化）。
    bool checkDependencies();
    std::vector<PackageInitializationPlan> getPackageInitializationOrder();
    bool loadPackagesInOrder();
```

### 2.3 影响评估

- **当前 Level（v3.x）追加 `[[deprecated]]`**：
  - **宿主 ds-editor-lite**：已 Grep 确认未调用这三个方法，编译期警告不会触发
  - **框架内部**：`Manager::initialize()` 不调用这三个方法（使用两阶段初始化流程），无影响
  - **ARCH-02 合规**：仅追加 `[[deprecated]]` 警告是 additive，不破坏源码兼容，不需递增 Level ✅
- **下一 Level（v4.x）私有化**：
  - private 化 public 方法为破坏性变更，须递增 Level 声明 ABI 断裂
  - 届时宿主侧已通过 v3.x `[[deprecated]]` 警告完成迁移，风险极低

---

## 3. ModelStep context 漏洞修复（VULN-1 / S5 场景，synthrt O-2）

### 3.1 漏洞根因

[ModelStep.cpp:44-61](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp)：

```cpp
auto g2pCate = spec->Mgr()->category("g2p");
auto g2pObj = g2pCate->getFirstObject(m_onnxG2pId);  // 裸 id，无 context 前缀
```

而 `createModuleTask`（[PackageManager.cpp:862](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp)）注册 task 时用 FQID：

```cpp
std::string fqid = ContextUtils::formatFqid(ContextKey(context, version), moduleId);
// 非默认 context: "SingerA:g2p-lstm-eng-official"
// 默认 context:   "g2p-lstm-eng-official"
ic.addObject(fqid, task);
```

**不匹配**：ModelStep 用裸 `m_onnxG2pId`（如 `g2p-lstm-eng-official`）查找，但声库私有 LstmG2p 注册为 `SingerA:g2p-lstm-eng-official`，查找不到 → `m_enabled = false` 静默降级。

### 3.2 修复方案：ModuleSpec::contextKey() + FQID 查找

**背景核实**：`ModuleSpec` 由 `ModuleCategory::parseSpec`/`loadSpec` 在包加载阶段创建（[PackageManager.cpp:175-325](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp)），此时**不知道 context**。context 信息在 `collectModuleMetadata` 阶段从 `contextModuleInfos` 获取，到 `createModuleTask`（[PackageManager.cpp:796-864](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp)）时才可用。因此 `contextKey` 必须在 `createModuleTask` 中注入，而非 spec 创建时。

**步骤 1**：给 `ModuleSpec::Impl` 增加 `contextKey` 成员

[Module_p.h](file:///D:/projects/language-manager/core/lib/Module/Module_p.h) `ModuleSpec::Impl`（第 18-46 行）新增：

```cpp
class LANGCORE_EXPORT ModuleSpec::Impl {
public:
    // ... 现有成员不变 ...
    LangCore::ContextKey contextKey;   // 新增：模块所属 context（createModuleTask 时注入）
};
```

> 注：`Module_p.h` 已 include `PackageManager_p.h`，后者间接 include `ContextUtils.h`，`ContextKey` 可用。若未间接 include，需显式 `#include <LangCore/Support/ContextUtils.h>`。

**步骤 2**：给 `ModuleSpec` 增加 `contextKey()` 公共 getter（additive，符合 ARCH-02）

[Module.h](file:///D:/projects/language-manager/core/include/LangCore/Module/Module.h) `ModuleSpec` 类（第 55-101 行）新增公共方法：

```cpp
class LANGCORE_EXPORT ModuleSpec {
public:
    // ... 现有方法不变 ...

    /// 返回此模块所属的 ContextKey。
    /// 默认 context 模块返回 ContextKey()（空 context + 空 version）。
    /// 声库私有模块返回 ContextKey(singerId, packageVersion)。
    /// 注：contextKey 在 createModuleTask 阶段注入，parseSpec/loadSpec 阶段为默认值。
    ContextKey contextKey() const;
};
```

[Module.cpp](file:///D:/projects/language-manager/core/lib/Module/Module.cpp) 实现：

```cpp
ContextKey ModuleSpec::contextKey() const {
    return _impl->contextKey;
}
```

**步骤 3**：在 `createModuleTask` 中注入 contextKey

[PackageManager.cpp:796-864](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) `createModuleTask` 中，在 `moduleSpec` 获取后、`addObject` 前，通过 friend 访问设置（`PackageManager` 是 `ModuleSpec` 的 friend，见 [Module.h:100](file:///D:/projects/language-manager/core/include/LangCore/Module/Module.h)）：

```cpp
const auto moduleSpec = pkg.moduleSpec(moduleInfo.type, moduleInfo.moduleId);
// 新增：注入 contextKey（friend 访问 _impl）
moduleSpec->_impl->contextKey = ContextKey(moduleInfo.context, moduleInfo.contextVersion);
// ... 后续 addObject 逻辑不变 ...
```

> **为何用 friend 而非公共 setter**：`ModuleSpec` 的 `_impl` 是 `protected`，`PackageManager` 是其 friend（Module.h:100），可直接访问。新增公共 setter 会暴露可变状态，破坏 ModuleSpec 的"创建后不可变"语义。friend 注入是框架内部行为，对外只暴露 getter。

**步骤 4**：修复 ModelStep 用 FQID 查找 + 声库优先/官方回退

[ModelStep.cpp:44-61](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp) `configure()` 中查找逻辑改为：

```cpp
#include <LangCore/Support/ContextUtils.h>

// ... configure() 中，替换原 getFirstObject(m_onnxG2pId) ...
const auto ctxKey = spec->contextKey();
const auto fqid = LangCore::ContextUtils::formatFqid(ctxKey, m_onnxG2pId);
auto g2pObj = g2pCate->getFirstObject(fqid);
if (!g2pObj && !ctxKey.isDefault()) {
    // 声库 context 找不到 → 回退默认 context（裸 id = 默认 context 的 FQID）
    g2pObj = g2pCate->getFirstObject(m_onnxG2pId);
}
```

**回退语义**（两级查找，对齐 synthrt O-2）：
1. **本 context 优先**：`formatFqid(ctxKey, id)` 在当前 context 查找。默认 context 下 `formatFqid` 返回裸 id（见 [ContextUtils.h:97-101](file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h)），等价于原逻辑。
2. **官方回退**：仅当当前 context 非默认且本 context 查不到时，回退默认 context 用裸 id 查找。这对应"声库自定义 G2P 可依赖官方模块"的运行时体现（与依赖解析阶段 Dep-2 回退一致）。
3. **找不到则禁用**：两级都查不到，`m_enabled = false`（保留现有 graceful degradation 行为）。

> **回退安全性**：若声库私有 ChainG2p 的 ModelStep `id` 填的是声库私有模块名（如 `g2p-lstm-singerA`），官方 context 无此 id，回退查不到，正确禁用。若 `id` 填的是官方模块名（如 `g2p-lstm-eng-official`），声库 context 无此 FQID，回退默认 context 查到官方模块，正确引用。

### 3.3 为什么选择 ModuleSpec::contextKey() 而非其他方案

| 方案 | 评估 | 结论 |
|------|------|------|
| A. ModuleSpec::contextKey() + FQID 查找（本方案） | additive、语义正确、最小改动 | ✅ 采用 |
| B. G2pContext 携带 ContextKey（插件内部） | G2pContext 仍需从 spec 获取 contextKey，问题未解决 | ❌ |
| C. ModelStep 遍历所有 context 尝试查找 | 可能匹配错误 context 的同名模块，违反隔离 | ❌ |
| D. 改 ModelStep 用 `Manager::task("g2p", ctx, id)` | ModelStep 持有 spec，需 spec→context 映射，仍需 A | ❌ |
| E. 让 ChainG2p 的 ModelStep 只能引用官方 LstmG2p | 限制声库能力，非通用方案 | ❌ 限制过强 |

方案 A 的优势：
- `ModuleSpec` 语义上就应知道"自己属于哪个 context"（这是合理的元数据）
- 框架内部已持有此信息（ModuleMetadata.context），只是未暴露
- additive 修改，所有现有插件无需改动（除非需要 context 感知，如 ModelStep）

### 3.4 验证场景

| 场景 | `ctxKey` | 第一级 FQID 查找 | 第二级官方回退 | 预期结果 |
|------|----------|-----------------|---------------|---------|
| 官方 ChainG2p + 官方 LstmG2p | `ContextKey()`（默认） | 裸 id（= FQID），查到 ✅ | 不触发（ctxKey.isDefault） | 行为不变 |
| 声库私有 ChainG2p + 声库私有 LstmG2p（S5） | `ContextKey("SingerA", v)` | `"SingerA@v:g2p-lstm-singerA"`，查到 ✅ | 不触发（已找到） | **S5 修复** |
| 声库私有 ChainG2p + 官方 LstmG2p（声库依赖官方） | `ContextKey("SingerA", v)` | `"SingerA@v:g2p-lstm-eng-official"`，查不到 | 裸 id `g2p-lstm-eng-official`，默认 context 查到 ✅ | 正确引用官方 |
| 声库私有 ChainG2p + 配置错误的 id（不存在） | `ContextKey("SingerA", v)` | 查不到 | 裸 id 回退也查不到 | `m_enabled=false` 正确禁用 |

---

## 4. 失败 context 可观测性（AD-F2 修复，synthrt O-1 必需）

### 4.1 现状

`PackageManager::Impl::contextStates`（`std::map<ContextKey, ContextState>`）私有，宿主无法查询。

### 4.2 新增公共 API

[PackageManager.h](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) 新增：

```cpp
/// Context 初始化状态。
/// v3 对齐修订：四态设计（原三态 Pending/Ready/Failed 追加 NotRegistered）。
/// 与 SingerInfo.resolutionState 三态（Resolved/Pending/Missing）是分层语义：
///   - SingerInfo 表达声库元数据解析状态（宿主侧）
///   - ContextState 表达框架 context 生命周期（框架侧）
enum class ContextState {
    Pending,       ///< 已注册但尚未初始化
    Ready,         ///< 初始化成功（至少一个包加载成功）
    Failed,        ///< 初始化失败（依赖缺失/环/Level 不兼容/driver 不可用，不阻塞其他 context）
    NotRegistered, ///< 未注册（不在 contexts() 枚举中）
};

class PackageManager {
public:
    // ... 现有方法不变 ...

    /// 查询指定 context 的初始化状态。
    /// 未注册的 context（不在 contexts() 枚举中）返回 NotRegistered。
    ContextState contextState(const ContextKey &ctxKey) const;

    /// 列出所有初始化失败的 context（不含默认 context，默认 context 失败会阻塞 initialize）。
    std::vector<ContextKey> failedContexts() const;
};
```

### 4.3 实现要点

- `contextState`：查 `impl->contextStates`，未命中（即未注册，不在 `contexts()` 枚举中）返回 `NotRegistered`（v3 对齐修订：原方案返回 `Pending`，已修正为 `NotRegistered` 以区分"未注册"与"已注册但未初始化"）
- `failedContexts`：遍历 `impl->contextStates`，收集 `Failed` 状态的 key（排除默认 context）

### 4.4 宿主侧使用方式（对齐 synthrt 05 §6.5 对策 2）

```cpp
// LaunchLanguageEngineTask 中，initialize() 后
langMgr->initialize();
for (const auto &failed : langMgr->failedContexts()) {
    qWarning() << "Voicebank G2P context failed:" << failed.toString().c_str();
}
```

### 4.5 与 synthrt O-3 的关系

synthrt O-3 建议"ONNX driver 可用性校验前移"：在 `Manager::initialize()` Phase 3 中对 ONNX-based class 的 task 增加 driver 可用性检查，缺失时标记 context 为 Failed（DriverUnavailable）。

本方案采用更轻量路径：
1. **宿主侧 `qFatal` 断言**（synthrt 05 §6.5 对策 1）保证 driver 已注册
2. **框架侧 `failedContexts()`**（本节）让宿主主动发现失败 context

不在框架侧前移 driver 校验，理由：
- 宿主侧断言已提供硬保护
- 框架侧前移需识别"ONNX-based class"清单，违反插件职责单一（[ARCH-01](../decisions/human-decisions.md)）
- `failedContexts()` 已能覆盖 driver 缺失导致的 task 失败（task `start()` 返回 `DriverUnavailable`）

---

## 5. G2pRes::isOk() 便利方法（AD-F4 + AD-F6 修复）

### 5.1 现状（mode 与 errorType 的实际语义）

核实 [Manager.cpp:361-500](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `convert()` 实现，`mode` 与 `errorType` 的实际组合：

| 来源 | mode | errorType | 含义 |
|------|------|-----------|------|
| C-2 空 lyric | `"skip"` | `NoError` | 合法跳过（空歌词） |
| C-3 空 g2pId | `"copy"` | `UnknownError` | 失败兜底 |
| C-4 非法 context | `"copy"` | `UnknownError` | 失败兜底 |
| C-5/C-6 task 未找到 | `"copy"` | `UnknownError` | 失败兜底 |
| 推理 start() 失败 | `"copy"` | `ModelInferenceFailed` | 失败兜底 |
| 结果类型错误 | `"copy"` | `UnknownError` | 失败兜底 |
| 插件返回（成功转换） | `"convert"` | `NoError` | 真正成功 |
| 插件返回（FallbackStep 原词保留） | `"copy"` | `NoError` | **合法的原词保留**（如标点、数字） |

**关键发现**：`mode == "copy"` **不等于**失败。插件可能返回 `mode=="copy" + NoError`（合法的原词保留，如 ChainG2p FallbackStep 对标点/数字原样返回）。`convert()` 内部产生的 copy 兜底**都**带非 NoError 的 errorType，但插件返回的 copy 可能是 NoError。

**正确判定**：
- **推理失败**（需回退）：`errorType != NoError`
- **推理成功**（含原词保留）：`errorType == NoError`
- **跳过**（空 lyric）：`mode == "skip"`

### 5.2 新增便利方法

[LangCommon.h](file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h) `G2pRes` 新增 inline 方法：

```cpp
struct G2pRes {
    // ... 现有字段不变 ...

    /// 是否未发生错误（含合法的原词保留，如标点/数字）。
    /// true 表示 errorType == NoError（mode 可能是 "convert" / "copy" / "skip"）。
    /// false 表示推理失败（errorType != NoError），调用方应考虑回退。
    /// 注：若需区分"真正转换"与"原词保留"，额外检查 mode == "convert"。
    bool isOk() const {
        return errorType == NoError;
    }

    /// 是否为推理失败兜底（需回退的场景）。
    /// 等价于 !isOk()。
    bool isFailed() const {
        return errorType != NoError;
    }
};
```

> **修正说明**：早期方案草案曾定义 `isOk()` 为 `errorType == NoError && mode == "convert"`，这是**错误的**——它会将插件合法返回的 `mode=="copy" + NoError`（标点原词保留）误判为失败。正确语义是 `isOk()` 仅看 `errorType`。

### 5.3 宿主侧使用

宿主侧 `G2pConvertRunner`（见 [synthrt 03 §7](file:///D:/projects/synthrt/docs/dspk-g2p-design/03-lite-singerinfo-propagation.md)）的回退判定应使用 `isFailed()`：

```cpp
// 旧：mode == "copy" && errorType != NoError && !context.empty()
// 新：res.isFailed() && !res.context.empty()
```

`isOk()` 用于"是否可用结果"判定，`mode` 用于区分处理方式（convert/copy/skip），两者独立。

---

## 6. initialize() 幂等性防护（AD-F8 修复，synthrt O-5 / L-2 硬约束）

### 6.1 现状

[Manager.cpp](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `initialize()` 未做"已初始化"防护，二次调用会重跑全流程（非增量），可能导致已加载 task 状态不一致。

synthrt [05 §5.2 L-2](file:///D:/projects/synthrt/docs/dspk-g2p-design/05-cross-module-initialization-design.md) 将此列为硬约束："`initialize()` 不可重复调用，框架未防护，宿主须自律"。

### 6.2 修复方案

[Manager.h](file:///D:/projects/language-manager/core/include/LangCore/Core/Manager.h) / [Manager.cpp](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `initialize()` 增加幂等防护：

```cpp
// Manager_p.h 或 Manager.cpp 内部
class Manager::Impl {
public:
    // ... 现有成员 ...
    bool m_initialized = false;   // 新增
};

// Manager.cpp
Error Manager::initialize() {
    if (_impl->m_initialized) {
        return Error{Error::AlreadyInitialized,
                     "Manager::initialize() has already been called"};
    }
    // ... 原有初始化逻辑 ...
    _impl->m_initialized = true;
    return Error::Success();
}
```

### 6.3 错误码新增

`Error::AlreadyInitialized` 为新增错误码（additive，符合 ARCH-02）。需在 [Error.h](file:///D:/projects/language-manager/core/include/LangCore/Support/Error.h) / [Error.cpp](file:///D:/projects/language-manager/core/lib/Support/Error.cpp) 新增枚举值。

### 6.4 影响评估

- **宿主侧**：synthrt L-2 已要求宿主不重复调用，返回错误码是更友好的失败反馈，不破坏现有行为
- **框架内部**：`m_initialized` 标志位防止重跑，避免 task 状态不一致
- **ARCH-02 合规**：新增错误码是 additive；`initialize()` 签名不变

---

## 7. 文档一致性修复

| 文档 | 问题 | 修复 |
|------|------|------|
| [VoiceBank-Scoped-Package-Design.md](../design/VoiceBank-Scoped-Package-Design.md) §11 | 残留 v4.0 过期文本 | ✅ 已删除（在文档清理中完成） |
| [Architecture-Overview.md](../Architecture-Overview.md) §3 | 测试目录描述错误 | ✅ 已修正为 `catch2/` + `common/` + `tst_langCore/`（在文档清理中完成） |
| `PackageManager::open()` 文档 | 暗示支持传递依赖 | 注释明确"不解析传递依赖，使用 addPackagePath + initialize 全流程" |
| [Issues-Tracker.md](../Issues-Tracker.md) | 引用旧 `refactoring/` 路径 | 更新指向 `refactor-plan/` |

---

## 8. 与设计原则的核对

| 设计原则（[human-decisions.md](../decisions/human-decisions.md)） | 本方案遵守情况 |
|------|--------------|
| ARCH-01 插件职责单一 | ModelStep 修复不引入重试/回滚，仅修正查找方式；不在框架侧前移 ONNX driver 校验 |
| ARCH-02 接口稳定，Level 锚定 | 所有 API 变更为 additive（新增方法/错误码），不改现有签名；当前 Level（v3.x）仅对 deprecated 方法追加 `[[deprecated]]` 警告（additive），私有化推迟到 v4.x 并递增 Level |
| ARCH-03 组合优于继承 | 不新增继承；ModuleSpec::contextKey 是组合元数据 |
| ARCH-04 相似模块统一设计 | 不涉及新插件 |
| ARCH-05 结构化控制流 | goto 消除已完成（§4 技术债） |
| ROBUST-01 Expected\<T\> | 新增 API 返回值类型符合规范 |
| ROBUST-02 异常边界隔离 | 不涉及第三方边界 |
| ROBUST-03 catch 禁止静默 | failedContexts 让失败可观测，不静默 |
| INFRA-01 ConfigAccessor | 不涉及 |
| INFRA-05 测试分级 | 新增多 context 端到端测试归入 L3（tst_langCore） |

详见 [05-design-principles-check.md](05-design-principles-check.md)。

---

## 9. 修复后的 S5 场景数据流

```
声库 SingerA 注册:
  addPackagePath("SingerA", v1.0, "/voicebank/SingerA/g2p_packages")
  → PackageManager 注册 SingerA context

initialize():
  Phase 2 加载 SingerA context:
    ChainG2p-SingerA 模块 → createModuleTask → spec.contextKey = ContextKey("SingerA", v1.0)
    LstmG2p-SingerA 模块  → createModuleTask → spec.contextKey = ContextKey("SingerA", v1.0)
    ObjectPool 注册:
      "SingerA@1.0:g2p-chain-singerA" → ChainG2p task
      "SingerA@1.0:g2p-lstm-singerA"  → LstmG2p task

convert({lyric, "g2p-chain-singerA", "SingerA", v1.0}):
  → 路由到 ChainG2p-SingerA task
  → ChainG2p pipeline 执行 ModelStep:
      spec->contextKey() = ContextKey("SingerA", v1.0)
      fqid = formatFqid(ctxKey, "g2p-lstm-singerA") = "SingerA@1.0:g2p-lstm-singerA"
      g2pCate->getFirstObject("SingerA@1.0:g2p-lstm-singerA") → ✅ 找到声库私有 LstmG2p
  → ModelStep 调用 LstmG2p 推理 → 返回音素
```

---

**关联文档**: [README.md](README.md) · [01-current-state-audit.md](01-current-state-audit.md) · [03-host-integration-contract.md](03-host-integration-contract.md) · [04-implementation-tasks.md](04-implementation-tasks.md) · [05-design-principles-check.md](05-design-principles-check.md) · [decisions/human-decisions.md](../decisions/human-decisions.md) · [synthrt 06 优化建议](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md)
