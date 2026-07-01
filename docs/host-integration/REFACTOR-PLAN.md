# language-manager G2P 重构方案（接口稳定化）

> 依据 synthrt `docs/g2p-design/` 权威设计（D10-D13）制定。
> 重构范围（D13）：文档重构 + 接口稳定化，不改业务逻辑与已验证实现。
> 框架核心（Manager/PackageManager/ContextState/S5）已验证，本次仅做接口字段对齐与文档同步。

## 一、重构依据

| 决策 | 内容 | 对本仓库的影响 |
| --- | --- | --- |
| D10 | 全链路字段统一为 `g2pContext`/`g2pContextVersion`，framework 新增 `g2pSource` | `G2pInput` + `G2pRes` 字段重命名 + 新增 `g2pSource`；`ContextKey` 保持不变（内部 API）；简化调用方 Layer 2 字段名映射层（Layer 1 类型转换层保留） |
| D11 | 宿主侧回退策略统一 `Never`-only | 框架 `convert()` 行为不变（本就无策略）；无需改动 |
| D12 | `G2pErrorType` 为本仓库独家定义的跨项目唯一错误契约 | 确认现状即可（已独家定义），调用方透传 |
| D13 | 重构范围 = 文档重构 + 接口稳定化 | 不改业务逻辑、不改已验证实现、不过度设计 |

## 二、接口稳定化任务

### 任务 T1：G2pInput + G2pRes 字段重命名 + 新增 g2pSource（D10）

**目标**：framework 对外 API（`G2pInput`/`G2pRes`）字段与调用方显示 struct 对齐，消除跨项目字段映射转换层。

**当前状态**（`core/include/LangCore/Base/LangCommon.h`）：

```cpp
// 第 23-34 行：G2pInput 当前字段
struct G2pInput {
    std::string lyric;
    std::string g2pId;
    std::string context;              // ← 待重命名为 g2pContext
    stdc::VersionNumber contextVersion;  // ← 待重命名为 g2pContextVersion
};

// 第 44-87 行：G2pRes 当前字段
struct G2pRes {
    std::string lyric;
    std::string g2pId;
    std::string context;              // ← 待重命名为 g2pContext
    stdc::VersionNumber contextVersion;  // ← 待重命名为 g2pContextVersion
    // ← 缺少 g2pSource
    std::string pronunciation;
    std::vector<std::string> candidates;
    std::string mode = "copy";
    G2pErrorType errorType = NoError;
};
```

**变更步骤**：

| 步骤 | 文件 | 变更 |
| --- | --- | --- |
| T1.1 | `core/include/LangCore/Base/LangCommon.h` | `G2pInput::context` → `g2pContext`；`G2pInput::contextVersion` → `g2pContextVersion`；构造函数同步更新 |
| T1.2 | `core/include/LangCore/Base/LangCommon.h` | `G2pRes::context` → `g2pContext`；`G2pRes::contextVersion` → `g2pContextVersion`；新增 `std::string g2pSource` 字段；构造函数同步更新（含 8 参数构造与 deprecated legacy 构造） |
| T1.3 | `core/lib/Core/Manager.cpp` | `convert()` 内所有 `G2pInput`/`G2pRes` 字段访问更新为 `g2pContext`/`g2pContextVersion`；详见 T2 |
| T1.4 | `plugins/G2ps/ChainG2p/internal/V1/TaskImpl.cpp` | G2pRes 构造点（第 51-68 行）字段访问更新（插件层不设置 `g2pContext`/`g2pContextVersion`/`g2pSource`，由 Manager 后置覆盖，仅需确保编译通过） |
| T1.5 | 全仓 Grep | 搜索 `\.context`/`\.contextVersion`（G2pInput/G2pRes 成员访问），更新为 `g2pContext`/`g2pContextVersion`；搜索 `G2pInput`/`G2pRes` 构造点，确保新字段有默认值或显式赋值 |

**不变项**：

| 结构体 | 字段 | 是否变更 | 理由 |
| --- | --- | --- | --- |
| `ContextKey` | `context` / `version` | **不变** | 框架内部 API，不对外暴露；`Manager::convert()` 内部做 `G2pInput.g2pContext` → `ContextKey.context` 的映射 |
| `G2pErrorType` | 枚举值 | **不变** | D12 确认为现状 |
| `G2pInput` 其他字段 | `lyric`/`g2pId` | **不变** | 无命名不一致 |
| `G2pRes` 其他字段 | `lyric`/`g2pId`/`pronunciation`/`candidates`/`mode`/`errorType` | **不变** | 无命名不一致 |

**验证**：
- 全仓编译通过（`context`/`contextVersion` 字段访问零遗漏）
- `tst_langCore` 端到端测试通过
- ds-editor-lite 对接：`G2pResult` 直接消费 `G2pOutcome` 字段（Layer 2 无字段名映射），Layer 1 类型转换层（std↔Qt）保留

**状态**：✅ 已完成（core + catch2 测试编译通过，ctest 100% 通过；plugins 无 `.context`/`.contextVersion` 字段访问，位置参数构造向后兼容）

---

### 任务 T2：Manager::convert() 填充 g2pSource + 更新所有构造点

**目标**：`convert()` 内所有 `G2pRes` 构造点填充 `g2pSource`，依据 `G2pInput.g2pContext` 判定来源。

**`g2pSource` 填充规则**：

| `G2pInput.g2pContext` | `g2pSource` 值 | 语义 |
| --- | --- | --- |
| 空（`""`） | `"official"` | 官方默认上下文 |
| 非空 | `"voicebank"` | 声库私有上下文 |

> 判定依据是 `G2pInput.g2pContext`（输入参数），不是 `ContextKey`。`ContextKey` 是框架内部查找键，语义与 `G2pInput.g2pContext` 一致（`convert()` 内部从 `G2pInput` 构造 `ContextKey`）。

**构造点清单**（`core/lib/Core/Manager.cpp` 第 367-506 行）：

| 赋值点 | 行号 | 场景 | `g2pSource` 填充 |
| --- | --- | --- | --- |
| C-2 | 391-394 | 空 lyric → skip | `item.g2pContext.empty() ? "official" : "voicebank"` |
| C-3 | 397-402 | 空 g2pId → copy | 同上 |
| C-4 | 405-410 | 非法 context → copy | 同上 |
| 任务未找到 | 441-469 | 任务查找失败 → copy | `group.g2pContext.empty() ? "official" : "voicebank"` |
| 推理失败 | 472-482 | start() 失败 → copy | 同上 |
| 成功路径 | 484-494 | 正常成功 → 覆盖赋值 | 同上（覆盖 task 返回的 res） |
| 非 G2pResultV1 类型 | 495-501 | result 类型不匹配 → copy | 同上 |

> 建议提取辅助函数 `static std::string g2pSourceFromContext(const std::string &g2pContext)`，避免重复。

**成功路径覆盖赋值**（第 491-492 行当前逻辑）：
```cpp
// 当前（重命名前）：
res.context = group.context;
res.contextVersion = group.contextVersion;

// 变更后：
res.g2pContext = group.g2pContext;
res.g2pContextVersion = group.g2pContextVersion;
res.g2pSource = group.g2pContext.empty() ? "official" : "voicebank";
```

**验证**：
- `tst_langCore` 端到端测试：私有上下文结果 `g2pSource == "voicebank"`，默认上下文结果 `g2pSource == "official"`
- 所有构造点 `g2pSource` 非空

**状态**：✅ 已完成（`g2pSourceFromContext` 辅助函数填充 7 个构造点；catch2 测试通过）

---

### 任务 T3：G2pErrorType 独家定义确认（D12）

**目标**：确认 `G2pErrorType` 为本仓库独家定义，调用方透传。

| 步骤 | 文件 | 检查 |
| --- | --- | --- |
| T3.1 | `core/include/LangCore/Base/LangCommon.h` | 确认 `G2pErrorType` 枚举定义完整（NoError/InvalidLyric/ModelInferenceFailed/PhonemeGenerationFailed/DriverUnavailable/UnknownError，第 36-43 行） |
| T3.2 | 全仓 Grep | 搜索 `enum.*G2pErrorType` 确认仅本仓库定义（ds-editor-lite 应仅 `using`/透传，无重定义） |
| T3.3 | 文档 | 在 `01-framework-capabilities.md` 与 `03-host-integration-contract.md` 中明确标注 D12 |

**已确认事实**（代码核对）：
- `G2pErrorType` 定义于 `LangCommon.h` 第 36-43 行
- ds-editor-lite `G2pOutcome.errorType` 类型为 `LangCore::G2pErrorType`（`G2pConvertRunner.h` 第 46 行），无 caller 级枚举

**调用方缺口**（非本仓库任务，记录供 ds-editor-lite 参考）：
- ds-editor-lite `G2pResult`（`G2pService.h` 第 17-28 行）当前**无 `errorType` 字段**，仅有 `fellBackToOfficial`
- D11 删除 `fellBackToOfficial` 后，G2pResult 失去错误指示
- D12 配套：ds-editor-lite 需给 `G2pResult` 新增 `LangCore::G2pErrorType errorType` 字段，从 `G2pOutcome.errorType` 透传
- 详见 ds-editor-lite REFACTOR-PLAN T1.7 + T4

**验证**：仅 `LangCommon.h` 定义 `G2pErrorType`；调用方直接使用 `LangCore::G2pErrorType`

**状态**：✅ 已完成（全仓 grep 确认：代码中仅 `LangCommon.h:36` 定义 `G2pErrorType`；`PRD-v2.0.md:270` 为文档示例非代码定义。D12 确认。调用方 ds-editor-lite 需新增 G2pResult.errorType，非本仓库任务）

---

### 任务 T4：convert() 路由透传确认（D11 配套）

**目标**：确认框架 `convert()` 无回退策略，D11 仅影响宿主侧。

| 步骤 | 文件 | 检查 |
| --- | --- | --- |
| T4.1 | `core/lib/Core/Manager.cpp` | 确认 `convert()` 仅按 `(g2pContext, g2pContextVersion, g2pId)` 路由，无 ToOfficial/二次调用逻辑（已确认：第 424 行注释 `// Lookup task: NO fallback to default context (C-6)`） |
| T4.2 | 文档 | 在 `03-host-integration-contract.md` 中明确：框架 `convert()` 无策略；回退策略（D11 后统一 Never）由宿主侧决定 |

**已确认事实**（代码核对）：
- `convert()` 不回退到默认 context（注释 C-6 明确）
- 无 `G2pFallbackPolicy`/`ToOfficial` 引用
- 唯一的回退机制：两步 ContextKey 查找（精确版本未命中时回退到 unversioned，同 context 名），非跨 context 回退

**验证**：框架代码无 `G2pFallbackPolicy`/`ToOfficial` 引用（本就不存在）

**状态**：✅ 已完成（全仓 grep 确认：`core/`、`plugins/` 代码无 `G2pFallbackPolicy`/`ToOfficial` 引用；仅 docs/ 与测试用例名出现。`convert()` 第 432 行保留 C-6 注释 `// Lookup task: NO fallback to default context`。D11 配套确认，无代码变更）

---

## 三、文档更新任务

### 任务 D1：同步 docs/host-integration/ 至 D10 字段命名

**目标**：现有 4 份文档（01-04）引用 `context`/`contextVersion` 字段名，需同步为 `g2pContext`/`g2pContextVersion`/`g2pSource`。

| 文档 | 更新点 |
| --- | --- |
| `01-framework-capabilities.md` | G2pRes/G2pInput 字段表：`context`→`g2pContext`、`contextVersion`→`g2pContextVersion`、新增 `g2pSource` 行；明确 D12（G2pErrorType 独家定义） |
| `02-context-isolation-mechanism.md` | convert() 路由模型描述：G2pRes/G2pInput 输出字段名同步；明确 `ContextKey` 保持 `context`/`version`（内部 API） |
| `03-host-integration-contract.md` | 错误处理表 + G2pRes 字段引用同步；明确 D11（框架无策略）+ D12（G2pErrorType 独家） |
| `04-test-design.md` | 测试用例中 G2pRes 字段断言同步；新增 `g2pSource` 字段验证用例 |

**执行时机**：T1（代码重命名）完成后，文档同步更新。

**状态**：✅ 已完成（4 份文档 + README 同步 D10 字段命名；01 G2pRes 字段表更新 g2pContext/g2pContextVersion/g2pSource + D12 标注；02 convert() 分组描述区分 G2pInput 对外字段与 ContextKey 内部 API；03 宿主契约重构无 G2pRes 字段引用需变更；04 测试用例字段引用同步 + 新增 g2pSource 验证用例说明；README 新增 D10-D13 决策摘要。ContextKey 内部 API 保持 context/version 不变）

---

### 任务 D2：tst_langCore 测试增强

**目标**：补齐 `g2pSource` 字段的端到端验证。

| 步骤 | 文件 | 变更 |
| --- | --- | --- |
| D2.1 | `tests/tst_langCore/main.cpp` | 在 G2P 转换结果输出中增加 `g2pSource` 字段打印（当前仅 `std::cout`，无断言） |
| D2.2 | `tests/tst_langCore/main.cpp` | 若有多上下文场景，验证私有上下文 `g2pSource=="voicebank"`、默认上下文 `g2pSource=="official"` |

**已知问题**（不在本次范围）：tst_langCore 存在硬编码路径、无断言宏、无多上下文场景。这些问题属测试基础设施改进，非 D10-D13 接口稳定化范围，后续单独处理。

**状态**：✅ 已完成（主 convert 路径结果输出增加 g2pContext/g2pSource 字段打印 + errorType 错误标记；默认上下文预期 g2pSource=="official"。性能测试路径 task->start() 不填充 g2pSource（由 Manager::convert 填充），未改。CLion inspection 无错误；tst_langCore 需 plugins+ONNX 运行时，字段访问编译正确性已由 catch2 测试间接验证）

---

## 四、任务依赖与执行顺序

```
T1（G2pInput/G2pRes 重命名 + g2pSource，D10）─┬─► T2（convert() 填充 g2pSource）
                                              ├─► D1（文档同步）
                                              └─► D2（tst_langCore 增强）

T3（G2pErrorType 确认，D12）─► D1（文档同步）
T4（convert() 确认，D11 配套）─► D1（文档同步）
```

- **T1 为核心任务**（有代码变更：字段重命名 + 新增字段）
- **T2 依赖 T1**（构造点字段访问需先重命名）
- **T2 可与 T1 合并执行**（同属一个提交单元）
- **T3、T4 为确认性任务**（预期无代码变更）
- **D1、D2 依赖 T1 完成**

## 五、不变量（重构后必须保持）

| 不变量 | 说明 |
| --- | --- |
| `(context, version, g2pId)` 三维路由 | 路由模型不变（`ContextKey` 内部仍用 `context`/`version`） |
| ContextState 四态（Pending/Ready/Failed/NotRegistered） | 状态机不变 |
| `initialize()` success-gated 幂等 | 初始化语义不变 |
| S5 两级查找（ModelStep FQID） | 模块依赖解析不变（插件层内部逻辑） |
| `collectError` 模式 | 单私有上下文失败不阻塞 |
| ONNX 驱动裸名注册 | 全局基础设施不变 |
| `G2pErrorType` 枚举值不变 | 现有 6 值不删除/不重排（保证二进制兼容） |
| `ContextKey` 字段名不变 | 框架内部 API，`context`/`version` 保持 |

## 六、向后兼容性

| 变更 | 兼容性影响 | 缓解 |
| --- | --- | --- |
| `G2pInput` 字段重命名 | **源码不兼容**（字段名变更） | ds-editor-lite 同步更新；框架为内部依赖，版本号递增 |
| `G2pRes` 字段重命名 | **源码不兼容**（字段名变更） | 同上 |
| 新增 `g2pSource` 字段 | **二进制兼容**（新增字段有默认值） | 默认 `""` 空字符串，旧调用方忽略 |
| `G2pErrorType` 枚举 | **不变** | 现有 6 值保持，未来新增值追加在末尾 |

## 七、风险与缓解

| 风险 | 缓解 |
| --- | --- |
| T1 字段重命名遗漏访问点 | 全仓 Grep `\.context`/`\.contextVersion` 逐点确认；编译验证 |
| `g2pSource` 填充逻辑错误 | 依据 `G2pInput.g2pContext` 判定，与路由模型一致；tst_langCore 端到端验证 |
| 跨项目同步导致临时编译失败 | T1 与 ds-editor-lite T2 协调执行；先 framework 重命名，后 caller 适配 |
| `ContextKey` 与 `G2pInput` 字段名不一致困惑 | 文档明确标注：`ContextKey` 为框架内部 API，`G2pInput`/`G2pRes` 为对外 API |

---

> 权威设计来源：`synthrt/docs/g2p-design/`（D1-D13 决策、SingerInfo/G2pRes 契约、跨模块集成设计）
> 关联文档：`ds-editor-lite/docs/g2p-architecture/REFACTOR-PLAN.md`
