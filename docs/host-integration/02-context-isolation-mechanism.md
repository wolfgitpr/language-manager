# 02 · 上下文隔离机制

本文档详述 LangCore 的上下文隔离（Context Isolation）机制，包括三维路由模型、ContextKey/FQID 格式、默认/私有上下文、状态生命周期、S5 漏洞修复（ModelStep 两级查找）、跨上下文依赖解析与命名校验。

> 2026-07-02 修订：修正 FQID 默认上下文格式（裸 moduleId，非 `:g2pId`）、ModelStep S5 回退（裸 moduleId）、L-4 保证机制（Phase 顺序非注册顺序）、补充 R-8 版本约束与跨上下文依赖回退方向。

## 1. 上下文模型：三维路由

LangCore 以 **`(context, version, g2pId)` 三维路由**（决策 D1）定位 G2P 模块：

- `context`：上下文标识。空字符串 `""` 表示官方默认上下文；非空（如 `singerId`）表示声库私有上下文。
- `version`：上下文版本。默认上下文必须为空；私有上下文必须非空（R-8）。
- `g2pId`：G2P 模块 ID。

### 1.1 ContextKey 复合键

`file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h` 第 17-54 行定义 `ContextKey`：

```cpp
struct ContextKey {
    std::string context;            // "" = 默认上下文
    stdc::VersionNumber version;    // isEmpty() = 无版本
    bool isDefault() const { return context.empty() && version.isEmpty(); }
};
```

### 1.2 FQID 格式（决策 D3）

`ContextUtils::formatFqid`（`ContextUtils.h:93-101`）按 `isDefault()` 分支生成 FQID：

```cpp
static std::string formatFqid(const ContextKey &ctxKey, const std::string_view &moduleId) {
    if (ctxKey.isDefault())
        return std::string(moduleId);           // ← 裸 moduleId，无冒号
    return ctxKey.toString() + ":" + std::string(moduleId);
}
```

| 上下文 | FQID 格式 | 示例 |
| --- | --- | --- |
| 默认上下文 | **裸 moduleId**（无冒号前缀） | `g2p-cmn-official` |
| 私有上下文（无版本） | `context:moduleId` | `SingerA:g2p-cmn-custom` |
| 私有上下文（带版本） | `context@version:moduleId` | `SingerA@2.0.0:g2p-cmn-custom` |

> ⚠️ **重要**：默认上下文的 FQID 是**裸 moduleId**（无冒号前缀），不是 `:moduleId`。这是 `formatFqid` 的 `isDefault()` 分支显式实现。FQID 解析测试见 `tests/catch2/tst_fqid.cpp`。

## 2. 默认上下文（context=""）

| 属性 | 说明 |
| --- | --- |
| 语义 | 官方 G2P，全局可见 |
| context 值 | 空字符串 `""` |
| version | **必须为空**（R-8 硬约束，`PackageManager.cpp:582-583`） |
| 注册方式 | `addPackagePath("", path)` |
| 角色 | 作为所有声库的兜底来源（决策 D2）；私有上下文依赖回退目标 |
| 初始化时机 | `initialize()` Phase 1 必然先处理（`Manager.cpp:74` 硬编码 `ContextKey("")`） |

默认上下文承载官方发布的 G2P 包，对所有私有上下文可见，是宿主侧 `ToOfficial` 回退策略的目标。

### 2.1 默认上下文失败的影响

默认上下文初始化失败 → `initialize()` 返回 Error（Ord-1），**整体中止**，所有上下文不可用。这是框架最严重的失败模式。

## 3. 私有上下文（context=singerId）

| 属性 | 说明 |
| --- | --- |
| 语义 | 声库自定义 G2P，仅对该声库可见 |
| context 值 | `singerId`（歌手标识） |
| version | **必须非空**（R-8 硬约束，`PackageManager.cpp:585-587`）；取声库声明版本 |
| 注册方式 | `addPackagePath(singerId, version, path)` |
| 隔离性 | 私有上下文间互不可见；可依赖回退到默认上下文 |
| 初始化时机 | `initialize()` Phase 2 遍历处理（`Manager.cpp:157-244`） |

### 3.1 私有上下文失败的影响

私有上下文初始化失败 → 标记 `ContextState::Failed`，`collectError` 模式收集错误，**继续处理下一个上下文**（不中止）。其他上下文不受影响。

### 3.2 context 命名校验

`ContextUtils::validateContextName`（`ContextUtils.h:112-134`）校验私有上下文名：

- 仅允许 `[A-Za-z0-9_.-]`
- 长度 ≤ 128 字符
- 不允许为空字符串（空字符串保留给默认上下文）

## 4. 跨上下文依赖回退

私有上下文在 `initialize()` Phase 2 依赖解析时，可将默认上下文模块作为回退（`file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp` 第 1130-1140 行）：

```cpp
// 非默认上下文：将默认上下文模块作为回退
if (!ctxKey.context.empty()) {
    auto defIt = impl.contextModuleInfos.find(ContextKey(""));
    if (defIt != impl.contextModuleInfos.end())
        fallback = defIt->second;
}
```

| 方向 | 允许? | 说明 |
| --- | --- | --- |
| 私有上下文 → 默认上下文 | ✅ | 依赖回退，初始化时解析 |
| 默认上下文 → 私有上下文 | ❌ | 禁止（C-6 不变式） |
| 私有上下文 ↔ 其他私有上下文 | ❌ | 禁止（C-6 不变式） |

> 注：此依赖回退发生在**初始化时**的依赖解析阶段，非运行时 `convert()` 的路由回退。`convert()` 本身无跨上下文回退（相邻输入按 `(context, version, g2pId)` 分组查找）。

## 5. S5 漏洞修复：ModelStep FQID 两级查找

### 5.1 背景

S5 漏洞：私有上下文中的 ModelStep 查找失败时，未正确降级到默认上下文，导致模型模块（如 LstmG2p）仅在默认上下文注册时无法被私有上下文的 ChainG2p 引用。

### 5.2 修复实现

`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp` 第 52-67 行：

```cpp
// 第一级：当前上下文按 FQID 查找
const auto ctxKey = spec->contextKey();
const auto fqid = LangCore::ContextUtils::formatFqid(ctxKey, m_onnxG2pId);
auto g2pObj = g2pCate->getFirstObject(fqid);
// 第二级：未命中且非默认上下文 → 回退默认上下文（裸 moduleId）
if (!g2pObj && !ctxKey.isDefault()) {
    g2pObj = g2pCate->getFirstObject(m_onnxG2pId);   // 裸 moduleId
}
if (!g2pObj) {
    // 优雅降级：DriverUnavailable
}
```

### 5.3 两级查找流程

```
┌──────────────────────────────┐
│  ModelStep 查找请求           │
│  (context, version, g2pId)   │
└──────────────┬───────────────┘
               │
               ▼
       ┌───────────────┐
       │ 第一级查找     │
       │ formatFqid()  │
       │ 当前上下文     │
       └───────┬───────┘
               │
        ┌──────┴──────┐
        ▼             ▼
      命中          未命中
        │             │
        ▼             ▼
    返回结果    ┌───────────────┐
                │ 非默认上下文?  │
                └───────┬───────┘
                        │
                 ┌──────┴──────┐
                 ▼             ▼
                是             否
                 │             │
                 ▼             ▼
        ┌───────────────┐  报错/降级
        │ 第二级查找     │
        │ 裸 moduleId    │
        │ (默认上下文)   │
        └───────┬───────┘
                │
         ┌──────┴──────┐
         ▼             ▼
       命中          未命中
         │             │
         ▼             ▼
     返回结果     DriverUnavailable
```

### 5.4 关键点

- 第二级回退使用**裸 moduleId**（`m_onnxG2pId`），与默认上下文的 FQID 格式一致（`formatFqid` 对 `isDefault()` 返回裸 moduleId）。
- 仅当当前上下文**非默认**时才触发回退（默认上下文已无更上层可回退）。
- 与宿主侧 `ToOfficial` 策略配合：FillLyric 场景下，两级查找的第二级即为官方兜底。

## 6. convert() 路由模型

`Manager::convert()`（`Manager.cpp:375-439`）按相邻输入分组路由：

1. 将相邻的 `(g2pContext, g2pContextVersion, g2pId)` 相同的输入分为一组（`G2pInput` 对外字段，D10）
2. 按两步 ContextKey 匹配查找 Task：
   - 精确匹配 `(context, version)`
   - 若未命中，回退到 `(context)` 无版本匹配
3. **无跨上下文回退**（C-6 不变式）：`convert()` 本身不跨上下文查找

> 跨上下文回退由宿主侧 `G2pConvertRunner`（`ToOfficial` 策略）实现：私有上下文 `convert()` 失败后，宿主用默认上下文参数再次调用 `convert()`。

## 7. 上下文状态生命周期

详见 [01-framework-capabilities.md](01-framework-capabilities.md) §2.2 的 ContextState 状态机。关键点：

- `addPackagePath` 后 → `Pending`
- `initialize()` Phase 1/2 完成 → `Ready` 或 `Failed`
- `NotRegistered` 是计算态（未注册）
- `Ready`/`Failed` 为终态（受 L-2 约束，修复需重启）

## 8. 约束总结

| 约束 | 内容 | 保证机制 |
| --- | --- | --- |
| L-1 | 自定义 G2P 仅启动时加载 | `addPackagePath` 不检查 `initialized`，但新路径不处理 |
| L-4 | 官方先于私有初始化 | `initialize()` Phase 1（默认）/ Phase 2（私有）顺序硬编码，与 `addPackagePath` 调用顺序无关 |
| R-8 | 默认上下文 version 必空；私有上下文 version 必非空 | `addPackagePath` 内校验（`PackageManager.cpp:582-587`） |
| C-6 | 私有→默认依赖回退允许；反向禁止 | `PackageManager.cpp:1130-1140` 仅对非默认上下文传入默认上下文回退 |

## 9. 源码引用

- ContextKey / FQID / 命名校验：`file:///D:/projects/language-manager/core/include/LangCore/Support/ContextUtils.h`
- Manager::convert 路由：`file:///D:/projects/language-manager/core/lib/Core/Manager.cpp`（第 375-439 行）
- 跨上下文依赖回退：`file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp`（第 1130-1140 行）
- R-8 版本约束：`file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp`（第 582-587 行）
- ModelStep S5 修复：`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`（第 52-67 行）
- FQID 测试：`file:///D:/projects/language-manager/tests/catch2/tst_fqid.cpp`
- 上下文隔离测试：`file:///D:/projects/language-manager/tests/catch2/tst_context_isolation.cpp`
