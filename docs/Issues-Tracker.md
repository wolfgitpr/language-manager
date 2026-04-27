# Language Manager 问题追踪

**版本**：2.1  
**日期**：2026-04-27  
**来源**：PRD v2.0 §14 设计评审记录 + 全量代码审计

---

## 严重程度定义

| 级别 | 含义 |
|------|------|
| 🔴 **P0 严重** | 导致数据错误、崩溃或安全问题，必须优先修复 |
| 🟠 **P1 中等** | 功能缺陷或可维护性问题，应在近期修复 |
| 🟡 **P2 低** | 性能优化或代码质量改进，可按计划安排 |
| 🟢 **P3 建议** | 长期改进方向，无紧迫性 |

## 修复风险定义

| 级别 | 含义 |
|------|------|
| ⚡ **低风险** | 改动局部，不影响其他模块，回归风险极小 |
| ⚠️ **中风险** | 涉及多个调用点或公共接口，需配套测试 |
| 🔥 **高风险** | 涉及核心架构或跨模块重构，需分阶段实施 |

---

## 1. 活跃 Bug

### 1.1 🔴 P0 · LstmG2p V2 成功推理结果的 mode 错误设为 "copy" — ✅ 已修复

**来源**：代码审计新发现  
**严重程度**：P0 严重 — 导致下游无法区分推理结果与原始拷贝  
**修复风险**：⚡ 低风险 — 单行修改  
**状态**：✅ 已修复。V2 TaskImpl.cpp 第 382 行现在正确使用 `"convert"`。

**问题**：`LstmG2p::Internal::V2::LstmG2pTaskImpl::start()` 第 380-382 行，成功推理生成音素后构造 `G2pRes` 时 `mode` 写死为 `"copy"`。而 V1 实现（第 287 行）正确使用了 `"convert"`。

```cpp
// V2 TaskImpl.cpp:380-382 — 已修复
g2pResult->g2pResult.emplace_back(LangCore::G2pRes{std::string(lyric), std::string(m_spec->id()),
                                                   std::string(), std::string(pronStr), std::vector<std::string>(),
                                                   std::string("convert")});  // ← 已修正为 "convert"

// V1 TaskImpl.cpp:286-287 — 正确
g2pResult->g2pResult = {LangCore::G2pRes{
    std::string(lyric), std::string(m_spec->id()), std::string(pronStr), ..., std::string("convert")}};
```

**影响**：所有通过 LstmG2p V2（即 Level 2 英语包）成功推理的结果，`mode` 字段都是 `"copy"` 而非 `"convert"`。ChainG2p 或前端如果根据 `mode` 判断是否使用推理结果，会产生错误行为。

**修复方案**：将 `plugins/G2ps/LstmG2p/internal/V2/TaskImpl.cpp` 第 382 行的 `"copy"` 改为 `"convert"`。

**相关文件**：
- `plugins/G2ps/LstmG2p/internal/V2/TaskImpl.cpp` 第 380-382 行
- 对比：`plugins/G2ps/LstmG2p/internal/V1/TaskImpl.cpp` 第 286-287 行（正确）

---

### 1.2 🟠 P1 · FormatStep::addSpaceBetweenPhones 命名与行为不符（§14.13）

**来源**：PRD §14.13  
**严重程度**：P1 中等 — 函数行为与名称不一致，可能导致调用方误用  
**修复风险**：⚠️ 中风险 — 需确认下游是否依赖当前行为

**问题**：函数名暗示"在音素间加空格"，实际行为是"在 alphanumeric 字符后遇到非空格非 alphanumeric 字符时插入空格"。

```cpp
// plugins/G2ps/ChainG2p/internal/Steps/FormatStep.cpp:52-71
std::string FormatStep::addSpaceBetweenPhones(const std::string &str) {
    std::string result;
    bool lastWasPhone = false;
    for (char c : str) {
        if (std::isalnum(c)) {
            result += c;
            lastWasPhone = true;
        } else if (lastWasPhone && c != ' ') {
            result += ' ';    // 在 alnum→非alnum 边界插入空格
            result += c;
            lastWasPhone = false;
        } else {
            result += c;
            lastWasPhone = false;
        }
    }
    return result;
}
```

**具体行为**：
- `"AH0 L OW1"` → 不变（已有空格）— ✅ 符合预期
- `"AH0L"` → `"AH0L"`（`'0'` 和 `'L'` 都是 `isalnum`，不插入空格）— ❌ 不符合"在音素间加空格"的语义
- `"AH0-L"` → `"AH0 -L"`（在 `'0'` 和 `'-'` 之间插入空格）— 行为正确但仅限此场景

**修复方案**：重命名函数为 `addSpaceBetweenAlnumAndSymbol()` 以反映真实行为，或重写逻辑使其真正支持音素间空格插入。需先明确实际使用场景的需求。

**相关文件**：`plugins/G2ps/ChainG2p/internal/Steps/FormatStep.cpp` 第 52-71 行

---

### 1.3 🟠 P1 · Session::close 中 goto 控制流影响可维护性（§14.17）

**来源**：PRD §14.17  
**严重程度**：P1 中等 — 不会崩溃但极难维护  
**修复风险**：⚠️ 中风险 — Session 是并发热路径，重构需保证线程安全

**问题**：`Session::open()` 和 `Session::close()` 使用了 `goto` 标签（`out_exists`、`out_search_hash`、`out_success`）进行控制流跳转。`close()` 在 `images.empty()` 时查找 `hash_size_map.find({group.size, group.hash})`，若 `open()` 走 `out_search_hash` 路径（path_map 命中但未计算 hash），`group.hash` 和 `group.size` 的值取决于首次创建该 group 时的路径——当前实现正确，但推理链过长。

```cpp
// plugins/Drivers/OnnxDriver/internal/Session.cpp
// open() 有 3 个 goto 标签: out_exists(505,531), out_search_hash(511,535), out_success(564,569)
// close() 有 1 个 goto 标签: out_success(597,614)
```

**影响**：代码正确性依赖于对 `path_map` / `hash_size_map` / `image_list` 三重索引的心智模型，后续修改极易引入回归 bug。

**修复方案**：提取 `tryReuseExistingSession()`、`createNewSession()`、`releaseSession()` 等子函数，用结构化控制流替代 goto。

**相关文件**：`plugins/Drivers/OnnxDriver/internal/Session.cpp` 第 468-620 行

---

## 2. 未完成功能

### 2.1 🟡 P2 · PackageManager::open 传递依赖加载未实现（§14.30）

**来源**：PRD §14.30  
**严重程度**：P2 低 — 当前所有包通过 `loadPackagesInOrder()` 拓扑加载，`open()` 的传递依赖逻辑不在主流程上  
**修复风险**：⚠️ 中风险 — 涉及包加载核心路径

**问题**：`PackageManager::Impl::open()` 第 145 行声明了 `llvm::SmallVector<PackageData *> dependencies`，但从未向其中添加元素。`closeDependencies` lambda（第 146-151 行）和 `pkg.linked = std::move(dependencies)`（第 251 行）操作的都是空向量。

```cpp
// core/lib/Core/PackageManager.cpp:145-151
llvm::SmallVector<PackageData *> dependencies;   // 始终为空
auto closeDependencies = [&dependencies, this] {
    for (auto it = dependencies.rbegin(); it != dependencies.rend(); ++it) {
        std::ignore = close(*it);
    }
};
// ...
pkg.linked = std::move(dependencies);  // line 251, 移动空向量
```

**影响**：通过 `open()` 单独打开一个包时，不会自动加载其依赖包。当前主流程使用 `loadPackagesInOrder()` 按拓扑序加载，不受影响。但如果未来有运行时动态加载单个包的需求，此功能缺失。

**修复方案**：根据包的 `dependencies` 声明，在 `open()` 中递归加载依赖包并填充 `dependencies` 向量。需防范循环依赖（已有 `pendingPackages` 机制）。

**相关文件**：`core/lib/Core/PackageManager.cpp` 第 145-151 行, 第 251 行

---

### 2.2 🟡 P2 · 静态插件链接选项未实现

**来源**：代码扫描  
**严重程度**：P2 低 — 选项默认 OFF，不影响正常构建  
**修复风险**：⚡ 低风险

**问题**：`plugins/CMakeLists.txt` 声明了 `option(LANGPLUGINS_ENABLE_STATIC_PLUGINS "Enable static plugin linking" OFF)`，但该选项未被任何构建逻辑消费。

**修复方案**：实现静态链接逻辑（需要修改 `LangPlugins_add_plugin` CMake 函数），或在确认不需要时移除该选项声明。

**相关文件**：`plugins/CMakeLists.txt`

---

## 3. 性能优化建议

### 3.1 🟡 P2 · LstmG2p V2 已完成样本继续参与解码（§14.9）

**来源**：PRD §14.9  
**严重程度**：P2 低 — 结果正确，仅浪费计算资源  
**修复风险**：⚠️ 中风险 — 涉及推理循环核心逻辑

**问题**：V2 批量解码时，虽然代码已追踪 `activeIndices`（第 240-249 行），但实际仍将 **全部** batchSize 的 tensor 送入解码器，已完成样本的 `predictedIds` 仍然参与 argmax 和下一步 decoder input 构造（第 330-333 行）。`finished` 标记仅阻止将输出写入 `allPredictions`。

```cpp
// plugins/G2ps/LstmG2p/internal/V2/TaskImpl.cpp:329-333
std::vector<int64_t> nextInputData(batchSize);
for (size_t i = 0; i < batchSize; ++i) {
    nextInputData[i] = predictedIds[i];  // 已完成样本仍然参与
}
```

**影响**：batch 内序列长度差异大时浪费明显。例如最短词 3 步完成、最长词 48 步，前者有 45 步无效 ONNX 推理计算。

**修复方案**：
- 方案 A（低复杂度）：对已完成样本的 `nextInputData[i]` 替换为 PAD token 而非 EOS/预测值
- 方案 B（高复杂度）：动态缩小 batch，仅对活跃样本构造 tensor，需 reshape

**相关文件**：`plugins/G2ps/LstmG2p/internal/V2/TaskImpl.cpp` 第 226-357 行

---

## 4. 长期目标

### 4.1 🟢 P3 · 继承链改组合（§14.7）

**来源**：PRD §14.7  
**严重程度**：P3 建议 — 当前可正常工作  
**修复风险**：🔥 高风险 — 涉及核心架构重构

**问题**：`Manager` → `PackageManager` → `PluginFactory` 三层 public 继承，导致接口暴露范围过大。§14.1 的成员遮蔽问题就是直接后果（已修复）。

**修复方案**：逐步将 `PluginFactory` 和 `PackageManager` 提取为独立组件，`Manager` 持有其实例而非继承。需配合 stdcorelib pimpl 约定（`__stdc_impl_t` / `__stdc_decl_t` 宏）和所有 `Impl` 类的改造分阶段进行。

**相关文件**：
- `core/include/LangCore/Core/Manager.h`
- `core/include/LangCore/Core/PackageManager.h`
- `core/include/LangCore/Core/PluginFactory.h`

---

## 5. 设计说明（非 Bug）

### 5.1 LstmG2p V1 仅处理首个单词（§14.8）

**结论**：设计如此，非 Bug。

V1 是逐词推理实现（Level 1）。代码第 200-201 行明确注释 `// For now, process only the first word`。V2 才是批量推理实现（Level 2）。当前英语包配置 `"level": 2`，运行时选择 V2。ChainG2p ModelStep 通过依赖声明 `"level": 2` 确保获取 V2 实例。

**潜在风险**：若包误配 LstmG2p 为 `level: 1` 但 ChainG2p ModelStep 的 `batchSize > 1`，V1 会丢弃首词以外的输入。建议 V1 的 `start()` 在输入大小 > 1 时返回明确错误。

**相关文件**：`plugins/G2ps/LstmG2p/internal/V1/TaskImpl.cpp` 第 200-201 行

---

## 6. 已修复问题（回归防护）

以下问题已修复，列出供回归测试参考。路径已经过代码审计核实。

| 编号 | 简述 | 已验证相关文件 |
|------|------|--------------|
| §14.1 ✅ | `Manager::Impl` 成员遮蔽 `PackageManager::Impl` 同名字段 | `core/lib/Core/Manager.cpp`, `core/lib/Core/Manager_p.h` |
| §14.2 ✅ | `VersionedTaskManager` 从模板简化为普通类，宏精简 | `core/include/LangCore/Task/VersionedTaskManager.h`, `core/lib/Task/Task.cpp` |
| §14.3 ✅ | 删除 Tarjan SCC，改用 Kahn 拓扑排序检测环 | `core/lib/Module/Dependency/DependencyGraph.cpp` 第 138-189 行 |
| §14.4 ✅ | `G2pErrorType` 22 个枚举精简为 6 个 | `core/include/LangCore/Base/LangCommon.h` |
| §14.5 ✅ | 删除 `WordInfo::metadata` 和 `G2pContext::m_metadata` 死代码 | `plugins/G2ps/ChainG2p/internal/Core/` |
| §14.6 ✅ | `PluginFactory::plugins<T>` static_assert 修正；`VersionUtils.cpp` catch 不再静默 | `core/include/LangCore/Core/PluginFactory.h` 第 47,59 行; `core/lib/Module/Dependency/VersionUtils.cpp` 第 303-309 行 |
| §14.10 ✅ | LstmG2p V1/V2 不再硬编码 `"eng"`，改用 `m_spec->id()` | V1: 第 206,252 等行; V2: 第 377,380 行 |
| §14.11 ✅ | MandarinG2p/CantoneseG2p 对非 convert mode 跳过转换 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.cpp` 第 115-127 行 |
| §14.12 ✅ | MandarinG2p getConfig() 缓存已实现 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.cpp` 第 56 行 `m_config = getConfig()` |
| §14.14 ✅ | `checkDependencies()` 收集全部不兼容错误后再返回 | `core/lib/Core/PackageManager.cpp` 第 446-474 行 |
| §14.15 ✅ | `Expected<T>` 默认构造通过 SFINAE 约束 | `core/include/LangCore/Support/Expected.h` |
| §14.16 ✅ | `pluginsDirty` 在 `scanPlugins()` 末尾正确清除 | `core/lib/Core/PluginFactory.cpp` 第 142 行 |
| §14.18 ✅ | `selectBestModules` 改用 index + key 避免指针失效 | `core/lib/Module/Dependency/DependencyResolver.cpp` |
| §14.19 ✅ | `Task::Mgr()` 检查 nullptr | `core/lib/Task/Task.cpp` |
| §14.20 ✅ | `VersionedTaskManager` 方法检查 `_impl` 是否为空 | `core/lib/Task/Task.cpp` |
| §14.21 ✅ | `Error::defaultMessage` 改用静态局部变量保证线程安全 | `core/lib/Support/Error.cpp` |
| §14.22 ✅ | `dependencyGraph` 重复调用前清空 | `core/lib/Core/PackageManager.cpp` 第 480 行 |
| §14.23 ✅ | `loadPackagesInOrder` 失败包不再静默跳过 | `core/lib/Core/PackageManager.cpp` 第 667-670 行 |
| §14.24 ✅ | MandarinG2p `initialize()` 失败正确返回错误 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.cpp` 第 58-60 行 |
| §14.25 ✅ | LstmG2p V1 先检查 Expected 再 `.take()` | `plugins/G2ps/LstmG2p/internal/V1/TaskImpl.cpp` 第 249-255 行 |
| §14.26 ✅ | MandarinG2p/CantoneseG2p 添加 cpp-pinyin 异常捕获 | `plugins/G2ps/MandarinG2p/internal/V1/TaskImpl.cpp` 第 131-146 行 |
| §14.27 ✅ | OnnxDriver `sessionRun()` 补充 `std::exception` catch | `plugins/Drivers/OnnxDriver/internal/Session.cpp` 第 347-356 行 |
| §14.28 ✅ | DsDict V1 改用 `input.as<DictInputV1>()` | `plugins/Dicts/DsDict/internal/V1/` |
| §14.29 ✅ | InferUtil `out = regexes` 移至循环后，修正 include guard | `plugins/Utils/InferUtil/` |

---

## 7. 问题总览

| 优先级 | 数量 | 概要 |
|--------|------|------|
| 🔴 P0 | 0 | LstmG2p V2 mode 错误（已修复，移入 §6） |
| 🟠 P1 | 2 | FormatStep 命名不符, Session goto |
| 🟡 P2 | 3 | 传递依赖未实现, 静态插件未实现, V2 EOS 性能 |
| 🟢 P3 | 1 | 继承链改组合 |
| ✅ 已修复 | 25 | 含 §14.12 getConfig 缓存、P0 LstmG2p V2 mode 修复 |

---

**文档版本**: 2.1  
**最后更新**: 2026-04-27
