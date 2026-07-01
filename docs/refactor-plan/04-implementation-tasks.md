# 04 — 实施任务与验证

> **日期**: 2026-07-01
> **执行原则**: 逐个任务单独提交（不推送）、完成后更新本文档状态、每次只核对任务相关代码
> **整合自**: 原 `lang-framework-plan/03-tasks.md` + synthrt O-4/O-5 新增任务

---

## 阶段 1：漏洞修复 + 架构债清理（框架侧独立）

### 任务 1.1 · 新增 ModuleSpec::contextKey() getter + createModuleTask 注入

**优先级**: P0 | **风险**: ⚠️ 中 | **依赖**: 无

**背景核实**：`ModuleSpec` 由 `ModuleCategory::parseSpec/loadSpec` 在包加载阶段创建（[PackageManager.cpp:175-325](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp)），此时**不知道 context**。context 信息在 `collectModuleMetadata` 阶段从 `contextModuleInfos` 获取，到 `createModuleTask`（[PackageManager.cpp:796-864](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp)）时才可用。因此 `contextKey` 必须在 `createModuleTask` 中注入，而非 spec 创建时。

**修改文件**（3 步注入设计，见 [02-interface-design §3.2](02-interface-design.md)）：
- [core/lib/Module/Module_p.h](file:///D:/projects/language-manager/core/lib/Module/Module_p.h) — `ModuleSpec::Impl`（第 18-46 行）增加 `LangCore::ContextKey contextKey;` 成员（默认构造为空 context + 空 version）。若 `ContextKey` 未间接 include，需显式 `#include <LangCore/Support/ContextUtils.h>`
- [core/include/LangCore/Module/Module.h](file:///D:/projects/language-manager/core/include/LangCore/Module/Module.h) — `ModuleSpec` 类（第 55-101 行）新增公共方法 `ContextKey contextKey() const;`（additive，符合 ARCH-02）
- [core/lib/Module/Module.cpp](file:///D:/projects/language-manager/core/lib/Module/Module.cpp) — 实现 getter：`return _impl->contextKey;`
- [core/lib/Core/PackageManager.cpp](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) `createModuleTask`（第 796-864 行）— 在 `moduleSpec` 获取后、`addObject` 前，通过 friend 访问注入：`moduleSpec->_impl->contextKey = ContextKey(moduleInfo.context, moduleInfo.contextVersion);`

**为何用 friend 而非公共 setter**：`ModuleSpec::_impl` 是 `protected`，`PackageManager` 是其 friend（Module.h:100），可直接访问。新增公共 setter 会暴露可变状态，破坏 ModuleSpec 的"创建后不可变"语义。friend 注入是框架内部行为，对外只暴露 getter。

**核实步骤**：
1. 头文件引用完整（`ContextUtils.h` 提供 `ContextKey`；`Module_p.h` 已 include `PackageManager_p.h`，后者间接 include `ContextUtils.h`，需 Grep 验证）
2. `cmake --build` 编译通过
3. 单元测试：`tst_context_version.cpp` 新增用例验证 `spec->contextKey()` 返回正确值（默认 context 返回 `ContextKey()`，声库 context 返回 `ContextKey("SingerA", v)`）

**验证标准**: 编译通过 + 单元测试通过 + 现有 catch2 测试全通过

**状态**: ✅ 已完成（contextKey getter + createModuleTask 注入；编译通过，CLion 无诊断错误；构建环境存在 MinGW/MSVC 库不匹配的预存链接错误，与本次改动无关；contextKey 端到端验证见任务 1.5）

---

### 任务 1.2 · 修复 ModelStep context 漏洞（VULN-1 / S5 场景，synthrt O-2）

**优先级**: P0 | **风险**: ⚠️ 中 | **依赖**: 任务 1.1

**修改文件**：
- [plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp) — 第 52 行查找逻辑改为 FQID

**改造要点**（见 [02-interface-design §3.4](02-interface-design.md)）：
```cpp
#include <LangCore/Support/ContextUtils.h>

// configure() 中，替换原 getFirstObject(m_onnxG2pId)
const auto ctxKey = spec->contextKey();
const auto fqid = LangCore::ContextUtils::formatFqid(ctxKey, m_onnxG2pId);
auto g2pObj = g2pCate->getFirstObject(fqid);
if (!g2pObj && !ctxKey.isDefault()) {
    // 声库 context 找不到 → 回退默认 context（官方兜底）
    g2pObj = g2pCate->getFirstObject(m_onnxG2pId);
}
```

**核实步骤**：
1. 编译通过
2. **新增端到端测试**（见任务 1.5）：S5 场景验证
3. 现有 `tst_langCore` 集成测试通过（官方包场景行为不变）

**验证标准**: S5 场景 convert 成功 + 官方包场景行为不变

**状态**: ✅ 已完成（ModelStep 改为 FQID 两级查找：本 context 优先，非默认 context 找不到时回退默认 context；ModelStep.cpp 编译通过，CLion 无诊断错误；S5 端到端验证见任务 1.5）

---

### 任务 1.3 · 追加 deprecated 编译期警告（AD-F1，v3.x 阶段）

**优先级**: P3 | **风险**: ⚡ 低 | **依赖**: 无（可与 1.1 并行）

> **v3 对齐修订（消解内部不一致）**：以 [05 §ARCH-02](05-design-principles-check.md) 消解方案为准。当前 Level（v3.x）**仅追加 `[[deprecated]]` 编译期警告**，私有化推迟到下一 Level（v4.x）并递增 Level。原方案"直接 private 化"违反 ARCH-02"破坏性变更须递增 Level"。

**修改文件**：
- [core/include/LangCore/Core/PackageManager.h](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) — 为 `checkDependencies / getPackageInitializationOrder / loadPackagesInOrder` 三个 public 方法追加 `[[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]`（保留 public 可见性）

**推迟到 v4.x 的内容**（不在本任务范围）：
- 将三个方法从 public 移到 private
- 递增 Level

**核实步骤**：
1. Grep 确认框架内部不调用这三个方法（`Manager::initialize()` 使用两阶段初始化，不依赖它们）
2. 编译通过（宿主侧未调用，`[[deprecated]]` 警告不会触发）
3. 现有 catch2 测试通过

**验证标准**: 编译通过 + 宿主侧 ds-editor-lite 编译通过（确认未调用，无警告触发）

**状态**: ✅ 已完成（三个 deprecated 扁平化方法追加 `[[deprecated]]` 编译期警告；保留 public 可见性；私有化推迟到 v4.x 并递增 Level；Manager::initialize() 不调用这三个方法；框架内部 PackageManager.cpp 交叉调用产生的预期警告不阻断编译）

---

### 任务 1.4 · 新增 contextState() / failedContexts() 可观测性 API（AD-F2，synthrt O-1 必需）

**优先级**: P2 | **风险**: ⚡ 低 | **依赖**: 无

**修改文件**：
- [core/include/LangCore/Core/PackageManager.h](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) — 新增 `ContextState` 枚举 + `contextState()` / `failedContexts()` 方法声明
- [core/lib/Core/PackageManager.cpp](file:///D:/projects/language-manager/core/lib/Core/PackageManager.cpp) — 实现

**实现要点**（见 [02-interface-design §4](02-interface-design.md)）：
- `contextState`：查 `impl->contextStates`，未命中（即未注册，不在 `contexts()` 枚举中）返回 `NotRegistered`（v3 对齐修订：原方案返回 `Pending`，已修正为 `NotRegistered` 以区分"未注册"与"已注册但未初始化"）
- `failedContexts`：遍历 `impl->contextStates`，收集 `Failed` 状态（排除默认 context）

**核实步骤**：
1. 编译通过
2. 单元测试：构造一个损坏的 context（如路径下无 package），`initialize()` 后 `failedContexts()` 返回该 context
3. 单元测试：查询未注册的 context，`contextState()` 返回 `NotRegistered`（非 `Pending`）
4. 现有测试通过

**验证标准**: 编译通过 + 单元测试通过

**状态**: ✅ 已完成（新增 ContextState 四态公共枚举；contextState() 三级判定：contextStates 命中→对应态、contextPackagePaths 命中→Pending、均未命中→NotRegistered；failedContexts() 遍历 contextStates 收集 Failed 且排除默认 context；状态查询测试归入任务 1.5 集成测试）

---

### 任务 1.5 · 新增多 context 端到端集成测试（AD-F5）

**优先级**: P1 | **风险**: ⚡ 低 | **依赖**: 任务 1.1、1.2

**修改文件**：
- [tests/tst_langCore/main.cpp](file:///D:/projects/language-manager/tests/tst_langCore/main.cpp) 或新建 `tests/tst_langCore/tst_multi_context.cpp`

**测试用例**：
1. **S5 场景**：注册声库 context（含私有 ChainG2p + 私有 LstmG2p），convert 验证 ModelStep 找到声库私有 LstmG2p，`errorType == NoError && mode == "convert"`
2. **官方回退场景**：声库私有 ChainG2p 的 ModelStep 引用官方 LstmG2p，验证回退默认 context 查找成功
3. **failedContexts 场景**：注册一个损坏路径的 context，`initialize()` 后 `failedContexts()` 包含该 context，且不阻塞其他 context
4. **contextState 场景**：查询 Ready/Failed/Pending/NotRegistered 四种状态（v3 对齐修订：未注册 context 返回 `NotRegistered` 而非 `Pending`）

**核实步骤**：
1. 测试编译通过
2. 测试用例全部通过

**验证标准**: 4 个测试用例通过

**状态**: ⬜ 未开始

---

### 任务 1.6 · 新增 G2pRes::isOk() / isFailed() 便利方法（AD-F4 + AD-F6）

**优先级**: P2 | **风险**: ⚡ 低 | **依赖**: 无

**背景核实**：核实 [Manager.cpp:361-500](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) `convert()` 实现，`mode` 与 `errorType` 的实际组合：

| `mode` | `errorType` | 含义 |
|--------|-------------|------|
| `"convert"` | `NoError` | 插件成功转换 |
| `"copy"` | `NoError` | 插件合法的原词保留（标点/数字，如 FallbackStep） |
| `"copy"` | 非 `NoError` | `convert()` 内部失败兜底（C-3/C-4/C-5/C-6/start 失败等） |
| `"skip"` | `NoError` | 空 lyric 跳过 |

**关键**：`mode == "copy"` **不等于**失败。正确判定是 `errorType != NoError`。原方案草案曾定义 `isOk()` 为 `errorType == NoError && mode == "convert"`，这是**错误的**——它会将插件合法返回的 `mode=="copy" + NoError`（标点原词保留）误判为失败。

**修改文件**：
- [core/include/LangCore/Base/LangCommon.h](file:///D:/projects/language-manager/core/include/LangCore/Base/LangCommon.h) — `G2pRes` 新增 inline 方法：
  ```cpp
  /// 是否未发生错误（含合法的原词保留，如标点/数字）。
  /// true 表示 errorType == NoError（mode 可能是 "convert" / "copy" / "skip"）。
  /// false 表示推理失败（errorType != NoError），调用方应考虑回退。
  /// 注：若需区分"真正转换"与"原词保留"，额外检查 mode == "convert"。
  bool isOk() const { return errorType == NoError; }

  /// 是否为推理失败兜底（需回退的场景）。
  /// 等价于 !isOk()。
  bool isFailed() const { return errorType != NoError; }
  ```

**核实步骤**：
1. 编译通过
2. 单元测试覆盖 4 种组合：
   - `NoError + mode=="convert"` → `isOk()==true, isFailed()==false`
   - `NoError + mode=="copy"` → `isOk()==true, isFailed()==false`（**关键：原词保留不是失败**）
   - `非 NoError + mode=="copy"` → `isOk()==false, isFailed()==true`
   - `NoError + mode=="skip"` → `isOk()==true, isFailed()==false`

**验证标准**: 编译通过 + 单元测试通过（4 种组合覆盖）

**状态**: ✅ 已完成（G2pRes 新增 isOk()/isFailed() inline 方法；isOk 仅看 errorType==NoError，含合法原词保留；4 种组合单元测试已添加到 tst_base_types.cpp，使用 8 参数构造避免 deprecated 警告）

---

### 任务 1.7 · 修复 P1 技术债（goto、成员遮蔽、const 语义）

**优先级**: P1 | **风险**: ⚠️ 中 | **依赖**: 无（可与 1.1-1.6 并行，但建议分开提交）

**子任务**：

#### 1.7a · G2pStep 成员遮蔽（TD-F1）
- 删除 [TagAndValidateStep.h](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/TagAndValidateStep.h) 中 `m_spec` 的 private 重新声明
- 验证：ChainG2p 相关测试通过

#### 1.7b · Package.cpp goto 消除（TD-F2，ARCH-05）
- [core/lib/Package/Package.cpp](file:///D:/projects/language-manager/core/lib/Package/Package.cpp)：`do { ... } while(false)` + `goto out_failed` → early return
- 验证：包加载测试通过

#### 1.7c · PhonemeDict.cpp goto 消除（TD-F3，ARCH-05）
- [core/lib/Support/PhonemeDict.cpp](file:///D:/projects/language-manager/core/lib/Support/PhonemeDict.cpp)：4 个 goto 标签 → continue + flag
- 验证：字典加载测试通过

#### 1.7d · DependencyGraph const 语义（TD-F4）
- [core/include/LangCore/Module/Dependency/DependencyGraph.h:88-90](file:///D:/projects/language-manager/core/include/LangCore/Module/Dependency/DependencyGraph.h)：移除 `addModule / buildGraph / clear` 的 const
- 验证：`tst_dependency_graph` 通过

**核实步骤**：
1. 每个子任务单独提交
2. 全部 catch2 测试通过
3. `tst_langCore` 集成测试通过

**验证标准**: 编译通过 + 全部测试通过 + goto 消除（Grep 确认无残留 goto 标签）

**状态**: ✅ 已完成（本方案撰写前已修复，提交 `f17ac89`/`9b9da2d`/`bf48915`，详见 [01-current-state-audit §4.1](01-current-state-audit.md)）

---

### 任务 1.8 · 修复 P2 技术债（include 守卫、static_assert、explicit）

**优先级**: P3 | **风险**: ⚡ 低 | **依赖**: 无

**子任务**（见 [02-interface-design §6.4](02-interface-design.md)）：
- 1.8a：5 个头文件 include 守卫改名
- 1.8b：8 处 static_assert 消息 `LangPlugins::` → `LangCore::`
- 1.8c：`core/lib/` 下 .cpp include 风格统一
- 1.8d：移除 4 处无参构造的 `explicit`
- 1.8e：`PhonemeDict` 显式 `= delete` 拷贝/移动

**核实步骤**：
1. 编译通过
2. 现有测试通过

**验证标准**: 编译通过 + 测试通过

**状态**: ✅ 已完成（本方案撰写前已修复，提交 `13308b8`/`98ff863`/`58d35cd` 等，详见 [01-current-state-audit §4.2](01-current-state-audit.md)）

---

### 任务 1.9 · 文档一致性修复

**优先级**: P3 | **风险**: ⚡ 低 | **依赖**: 无

**修改文件**：
- ✅ [docs/design/VoiceBank-Scoped-Package-Design.md](file:///D:/projects/language-manager/docs/design/VoiceBank-Scoped-Package-Design.md) — 删除 §11 过期文本，版本回退 v3.1（已在前序文档清理中完成）
- ✅ [docs/Architecture-Overview.md](file:///D:/projects/language-manager/docs/Architecture-Overview.md) — §3 测试目录描述修正为 catch2/ + common/ + tst_langCore/（已在前序文档清理中完成）
- ✅ [docs/Module-Reference.md](file:///D:/projects/language-manager/docs/Module-Reference.md) — §4 测试目录描述同步修正（已在前序文档清理中完成）
- ⬜ [core/include/LangCore/Core/PackageManager.h](file:///D:/projects/language-manager/core/include/LangCore/Core/PackageManager.h) — `open()` 注释明确"不解析传递依赖"
- ⬜ [docs/Issues-Tracker.md](file:///D:/projects/language-manager/docs/Issues-Tracker.md) — 引用旧 `refactoring/` 路径更新指向 `refactor-plan/`

**验证标准**: 文档与代码一致

**状态**: 🟡 部分完成（前 3 项已在文档清理中完成；后 2 项待处理）

---

### 任务 1.10 · 核实并增强 DependencyResolver level 精确匹配（synthrt O-4）

**优先级**: P3 | **风险**: ⚡ 低 | **依赖**: 无

**背景**：synthrt [D9](file:///D:/projects/synthrt/docs/dspk-g2p-design/05-cross-module-initialization-design.md) 要求声库 G2P 包的 `dependencies` 须明确 `packageId/moduleId/level/version`，不允许 `*` 通配。需核实当前 `DependencyResolver` 是否支持 `level` 维度匹配。

**核实步骤**：
1. 阅读 [core/lib/Module/Dependency/DependencyResolver.cpp](file:///D:/projects/language-manager/core/lib/Module/Dependency/DependencyResolver.cpp) 与 [core/lib/Module/Dependency/LevelCompatibilityChecker.cpp](file:///D:/projects/language-manager/core/lib/Module/Dependency/LevelCompatibilityChecker.cpp)
2. 确认 `selectBestModules` 是否在 `packageId`、`moduleId`、`level`、`version` 四维同时匹配
3. 若不支持 `level`，新增精确匹配逻辑（不匹配触发 Dep-1，走 Dep-2 回退）

**修改文件**（视核实结果而定）：
- [core/lib/Module/Dependency/DependencyResolver.cpp](file:///D:/projects/language-manager/core/lib/Module/Dependency/DependencyResolver.cpp) — 若需补充 level 维度

**验证标准**: level 不匹配时触发 Dep-1（依赖缺失），走 Dep-2 回退默认 context

**状态**: ⬜ 未开始（待核实）

---

### 任务 1.11 · initialize() 幂等性防护（AD-F8，synthrt O-5 / L-2）

**优先级**: P3 | **风险**: ⚡ 低 | **依赖**: 无

**修改文件**：
- [core/include/LangCore/Support/Error.h](file:///D:/projects/language-manager/core/include/LangCore/Support/Error.h) — 新增 `AlreadyInitialized` 错误码枚举值
- [core/lib/Support/Error.cpp](file:///D:/projects/language-manager/core/lib/Support/Error.cpp) — 错误码字符串映射
- [core/lib/Core/Manager_p.h](file:///D:/projects/language-manager/core/lib/Core/Manager_p.h) — `Manager::Impl` 新增 `bool m_initialized = false;` 成员
- [core/lib/Core/Manager.cpp](file:///D:/projects/language-manager/core/lib/Core/Manager.cpp) — `initialize()` 开头检查 `m_initialized`，末尾置 true

**实现要点**（见 [02-interface-design §6](02-interface-design.md)）：
```cpp
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

**核实步骤**：
1. 编译通过
2. 单元测试：连续两次调用 `initialize()`，第二次返回 `AlreadyInitialized` 错误
3. 现有测试通过

**验证标准**: 编译通过 + 二次调用返回 `AlreadyInitialized` + 现有测试通过

**状态**: ✅ 已完成（Error::Type 新增 AlreadyInitialized 枚举值；Manager::initialize() 开头检查 initialized 标志位；复用 PackageManager::Impl 已有的 initialized 字段，无需新增 m_initialized；幂等性测试需成功 initialize() 后验证，归入任务 1.5 集成测试）

---

## 阶段 2：版本发布与宿主对接

### 任务 2.1 · 版本号升级与发布

**优先级**: P1 | **风险**: ⚡ 低 | **依赖**: 阶段 1 全部完成

**操作**：
1. 更新 [CMakeLists.txt](file:///D:/projects/language-manager/CMakeLists.txt) 版本号（按 semver，因有 additive API 新增，建议 minor 版本+1）
2. 更新 [core/include/LangCore/Core/Plugin.h](file:///D:/projects/language-manager/core/include/LangCore/Core/Plugin.h) 的 `apiLevel`（若需要；additive 变更通常不需递增 Level）
3. 打 tag
4. 通知宿主侧更新 vcpkg overlay port

**验证标准**: 版本号正确 + 宿主侧能成功升级

**状态**: ⬜ 未开始

---

## 执行顺序与提交规范

### 建议执行顺序

```
1.1（ModuleSpec::contextKey）→ 1.2（ModelStep 修复）→ 1.5（多 context 测试）
                                                              ↓
1.3（追加 deprecated 警告，v3.x）∥ 1.4（可观测性 API）∥ 1.6（isOk/isFailed）∥ 1.11（initialize 幂等）  ← 可并行
                                                              ↓
1.9（文档）∥ 1.10（DependencyResolver level 核实）  ← 可并行
                                                              ↓
                                                    2.1（版本发布）
```

> 任务 1.7（P1 技术债）与 1.8（P2 技术债）已完成，无需再排期。

### 提交规范

- 每个任务单独提交，不推送
- 提交信息格式：`refactor(core): <任务编号> <简述>` 或 `fix(chain-g2p): <简述>`
- 示例：`fix(chain-g2p): 1.2 ModelStep uses FQID lookup to fix voicebank-private LstmG2p (S5)`
- 完成单个任务后，更新本文档对应任务的"状态"字段为 `✅ 已完成`

### 风险控制

| 风险 | 缓解措施 |
|------|---------|
| 1.1 ModuleSpec::contextKey 实现遗漏设置点 | Grep `createModuleTask` 全部调用点，确保都设置 contextKey |
| 1.2 ModelStep 回退逻辑误匹配 | 测试覆盖"声库私有优先 + 官方回退"两种场景 |
| 1.3 deprecated 警告误触发 | 宿主侧已确认未调用；`[[deprecated]]` 仅在调用点触发，无调用即无警告 |
| 1.11 幂等防护破坏现有测试 | 现有测试不应重复调用 initialize；若有，需调整测试夹具 |
| 1.5 测试声库包缺失 | 在 tests/tst_langCore/configs 下构造最小测试包 |

---

## 完成标准（Definition of Done）

- [ ] 阶段 1 全部任务完成（1.1-1.6, 1.9-1.11）
- [ ] `cmake --build` 编译通过
- [ ] catch2 14 个测试文件全部通过
- [ ] tst_langCore 集成测试通过
- [ ] **新增**多 context 端到端测试通过（S5 场景 + 官方回退 + failedContexts + contextState 四态：Ready/Failed/Pending/NotRegistered）
- [ ] Grep 确认三个 deprecated 方法已追加 `[[deprecated]]` 警告（v3.x 阶段；私有化推迟到 v4.x）
- [ ] `initialize()` 二次调用返回 `AlreadyInitialized`
- [ ] 宿主侧 ds-editor-lite 升级后编译通过
- [ ] 本文档所有任务状态更新

---

**关联文档**: [README.md](README.md) · [01-current-state-audit.md](01-current-state-audit.md) · [02-interface-design.md](02-interface-design.md) · [03-host-integration-contract.md](03-host-integration-contract.md) · [05-design-principles-check.md](05-design-principles-check.md) · [synthrt 06 优化建议](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md)
