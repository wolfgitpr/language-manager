# 02 · 上下文隔离机制

本文档详述 LangCore 的上下文隔离（Context Isolation）机制，包括三维路由模型、默认/私有上下文、状态生命周期、S5 漏洞修复（ModelStep 两级查找）、跨上下文依赖解析与命名校验。

## 1. 上下文模型：三维路由

LangCore 以 **`(context, version, g2pId)` 三维路由**（决策 D1）定位 G2P 模块：

- `context`：上下文标识。空字符串 `""` 表示官方默认上下文；非空（如 `singerId`）表示声库私有上下文。
- `version`：上下文版本，支持同上下文多版本并存。
- `g2pId`：G2P 模块 ID。

三者组合形成 **FQID（Fully Qualified ID）**，格式为 `context:g2pId`（决策 D3）：

- 私有上下文：`SingerA:g2p-cmn-custom`
- 默认上下文：`:g2p-cmn-official`（context 前缀为空）

> FQID 解析测试见 `tests/catch2/tst_fqid.cpp`。

## 2. 默认上下文（context=""）

| 属性 | 说明 |
| --- | --- |
| 语义 | 官方 G2P，全局可见 |
| context 值 | 空字符串 `""` |
| 注册方式 | `addPackagePath("", path)` |
| 角色 | 作为所有声库的兜底来源（决策 D2） |
| 加载顺序 | **必须最先注册**（约束 L-4） |

默认上下文承载官方发布的 G2P 包，对所有私有上下文可见，是 `ToOfficial` 回退策略的目标。

## 3. 私有上下文（context=singerId）

| 属性 | 说明 |
| --- | --- |
| 语义 | 声库范围隔离的自定义 G2P |
| context 值 | 声库标识（如 `SingerA`） |
| 注册方式 | `addPackagePath("SingerA", path)` 或 `addPackagePath("SingerA", version, path)` |
| 隔离性 | 与其他私有上下文互不可见，与默认上下文隔离 |
| 加载顺序 | 在官方上下文之后注册（约束 L-4） |

私有上下文允许声库提供自定义 G2P 模块，且其命名可与官方 G2P 的 `g2pId` 相同而不冲突（通过 context 前缀区分）。

### 3.1 隔离示例

```
默认上下文 ("")        私有上下文 ("SingerA")
├── g2p-cmn-official   ├── g2p-cmn-custom      (SingerA 专属)
├── g2p-eng-official   └── g2p-cmn-official    (覆盖官方同名)
└── g2p-jpn-official
```

- 在 `SingerA` 上下文中请求 `g2p-cmn-official`：优先命中私有上下文的同名模块（覆盖）。
- 在 `SingerA` 上下文中请求 `g2p-eng-official`：私有上下文未命中，由 ModelStep 两级查找回退到默认上下文（见第 5 节）。

## 4. ContextState 生命周期

上下文状态由 `PackageManager` 维护（`PackageManager.h:33-38`），四态如下：

```cpp
enum class ContextState { Pending, Ready, Failed, NotRegistered };
```

### 4.1 状态转移

```
  NotRegistered ──addPackagePath──► Pending
                                        │
                          initialize()  │ 扫描 + 依赖解析
                              ┌─────────┴─────────┐
                              ▼                   ▼
                            Ready               Failed
                          (可用)              (依赖失败)
```

| 转移 | 触发 | 说明 |
| --- | --- | --- |
| `NotRegistered → Pending` | 宿主调用 `addPackagePath` | 注册上下文路径 |
| `Pending → Ready` | `initialize()` | 扫描完成且依赖解析成功 |
| `Pending → Failed` | `initialize()` | 扫描完成但依赖解析失败 |
| — | — | `Ready`/`Failed` 为终态（受 L-2 幂等约束，不可回退） |

### 4.2 宿主查询语义

- `NotRegistered`：上下文未注册，宿主应视为「无此上下文」，调用 `convert` 时该上下文无效。
- `Pending`：包正在扫描，宿主不应在 `initialize()` 完成前调用 `convert`。
- `Ready`：上下文可用，`convert` 可正常工作。
- `Failed`：依赖解析失败，该上下文的 `convert` 将返回失败结果（`errorType != NoError`）。

## 5. S5 漏洞修复：ModelStep 两级查找

### 5.1 漏洞背景

S5 漏洞场景：某 `g2pId` 在私有上下文中被引用（例如 ChainG2p 的某一步骤引用了一个模型模块），但该模型模块 **仅在默认上下文中注册**。在修复前，私有上下文查找该 `g2pId` 会直接失败，导致转换错误。

### 5.2 修复方案（决策 D4）

`plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp` 第 53–60 行实现 **FQID 两级查找**：

1. **第一级（当前上下文）**：尝试 `context:g2pId`
   - 例如在 `SingerA` 上下文中查找 `SingerA:g2p-cmn-custom`
2. **第二级（默认上下文回退）**：若第一级未命中，尝试 `:g2pId`（空 context 前缀）
   - 例如回退查找 `:g2p-cmn-official`

```
查找流程：
  当前 context = "SingerA", g2pId = "model-xyz"
     │
     ▼
  ① 查找 "SingerA:model-xyz"  ──命中──► 使用私有上下文模块
     │
     │ 未命中
     ▼
  ② 查找 ":model-xyz"         ──命中──► 使用默认上下文模块（回退）
     │
     │ 未命中
     ▼
  ③ 失败（errorType != NoError）
```

### 5.3 设计理由

模型模块（如 ONNX 模型封装）属于 **共享基础设施**，通常只在官方默认上下文注册一次。强制每个私有上下文重复注册模型模块既浪费又不现实。两级查找保证：

- 私有上下文可覆盖同名模型模块（第一级命中优先）。
- 未覆盖时自动回退到默认上下文的共享模型（第二级）。
- 两者皆无时明确失败（第三级）。

### 5.4 与 ONNX 驱动注册的关系

注意区分两个层次的「全局化」：

| 层次 | 机制 | 注册位置 |
| --- | --- | --- |
| 驱动层（ONNX Driver） | 裸名 `g2pOnnxDriver` 注册到 `driver` 分类，不参与上下文隔离（D5） | `category("driver")` |
| 模型模块层（ModelStep） | FQID 两级查找，私有未命中回退默认上下文（D4） | G2P 包内模块 |

ONNX 驱动是底层推理引擎，模型模块是 G2P 流程中引用的具体模型；前者完全全局，后者通过两级查找实现「私有优先 + 默认兜底」。

## 6. 跨上下文依赖解析

### 6.1 依赖解析范围

`DependencyResolver` 在解析模块依赖时遵循上下文隔离原则：

- **私有上下文内部的依赖**：在当前私有上下文内解析。
- **回退到默认上下文**：通过 ModelStep 两级查找机制，模型模块可回退到默认上下文。
- **不跨私有上下文**：`SingerA` 的依赖不会解析到 `SingerB` 的私有上下文。

### 6.2 解析失败处理

当依赖解析失败时（如循环依赖、缺失依赖）：

- 该上下文状态置为 `Failed`。
- 错误通过 `collectError` 模式收集（决策 D7），不中断其他上下文的解析。
- 宿主调用 `convert` 时，`Failed` 上下文返回 `errorType != NoError` 的结果。

## 7. 上下文命名校验

上下文名（`context` 字段）需满足校验规则（详见 `tests/catch2/tst_context_validation.cpp`）：

| 规则 | 要求 |
| --- | --- |
| 允许字符 | `[A-Za-z0-9_.-]` |
| 最大长度 | 128 字符 |
| 默认上下文 | 空字符串 `""`（特殊值，不受字符规则约束） |

### 7.1 校验失败处理

- 不合法的上下文名将导致注册失败或上下文不可用。
- 宿主应在调用 `addPackagePath` 前对声库标识进行规范化，避免非法字符。

## 8. 现有上下文测试参考

`tests/catch2/` 下已有以下上下文相关测试，可作为新测试的参考模式：

| 测试文件 | 覆盖内容 |
| --- | --- |
| `tst_context_convert.cpp` | G2pInput/G2pRes 的 context 字段校验 |
| `tst_context_isolation.cpp` | 上下文隔离 + 默认回退（使用 mock ModuleMetadata + DependencyResolver） |
| `tst_context_dedup.cpp` | 模块去重 |
| `tst_context_version.cpp` | 版本化上下文，FQID 格式 |
| `tst_context_validation.cpp` | 上下文名校验 |
| `tst_fqid.cpp` | FQID 解析（如 `SingerA:g2p-cmn-custom`） |

> 新测试设计详见 [04-test-design.md](04-test-design.md)。

## 9. 源码引用

- PackageManager / ContextState：`file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h`
- 公共类型（G2pRes / G2pInput）：`file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h`
- ModelStep S5 修复：`file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp`
- 上下文隔离测试：`file:///D:/projects/language-manager/tests/catch2/tst_context_isolation.cpp`
- FQID 解析测试：`file:///D:/projects/language-manager/tests/catch2/tst_fqid.cpp`
- 上下文校验测试：`file:///D:/projects/language-manager/tests/catch2/tst_context_validation.cpp`
