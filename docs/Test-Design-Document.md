# Language Manager 测试设计文档

**版本**：2.0  
**日期**：2026-04-27  
**关联 PRD**：PRD-v2.0.md §10

---

## 1. 目标与原则

### 1.1 目标

建立分层、可维护的自动化测试体系，覆盖以下维度：

- **正确性**：核心类型、依赖解析、插件加载、G2p 转换各环节的正确行为
- **错误路径**：配置错误、依赖缺失/循环/不兼容、插件加载失败等异常情况
- **跨包依赖**：多 Package 互相依赖导致的复杂依赖场景
- **边界条件**：空输入、极值、格式异常等边界情况

### 1.2 原则

| 原则 | 说明 |
|------|------|
| 分模块组织 | 每个测试目标对应一个独立子目录，便于 review 和选择性运行 |
| 自包含 | 测试使用的 fixture 数据（JSON configs、mock packages）随测试代码存放，不依赖外部绝对路径 |
| 有断言 | 每个测试用例必须有明确的 pass/fail 判定，禁止 print-only |
| CTest 集成 | 所有测试通过 `add_test()` 注册，可用 `ctest --test-dir build -C Debug` 统一运行 |
| 无外部依赖 | 单元测试不依赖 ONNX Runtime、cpp-pinyin 等外部库；集成测试可选择性跳过 |

### 1.3 测试框架选择

**已实施状态**：所有 L1/L2 测试已迁移至 **Catch2 v2.13.10**（单头文件模式，直接携带 `catch.hpp`），不依赖 Qt Test。`tst_langCore/` 保留为独立 benchmark + 最小使用示例（不使用 Catch2）。

**选择理由**：
- Catch2 单头文件部署，零构建依赖
- `TEST_CASE` + `REQUIRE`/`CHECK` 宏提供现代化断言语义
- 原生支持 test case 名称、section、表达式模板
- 与 CTest 天然集成（`add_test()`）

---

## 2. 测试层次与目录结构

### 2.1 四层测试

```
L1  单元测试      纯逻辑验证，不加载任何插件或资源
L2  组件测试      测试核心子系统（依赖解析、包管理），使用 mock 数据
L3  插件测试      加载真实插件 DLL，验证插件交互
L4  端到端测试    完整 G2p 管线（Split→Tag→Convert），需要真实资源包
```

### 2.2 目录规划

```
tests/
├── CMakeLists.txt                  # 注册所有子目录
├── common/                         # 测试公共设施
│   ├── TestConfig.h.in             # CMake configure_file 模板（注入路径）
│   └── TestUtils.h                 # 公共辅助函数
│
├── tst_support/                    # L1: Expected, Error, ConfigAccessor, ValidationChain
│   ├── CMakeLists.txt
│   ├── tst_error.cpp
│   ├── tst_expected.cpp
│   ├── tst_config_accessor.cpp
│   └── fixtures/
│       └── *.json                  # 测试用 config JSON
│
├── tst_version/                    # L1: VersionRange, VersionResolver
│   ├── CMakeLists.txt
│   ├── tst_version_range.cpp
│   └── tst_version_resolver.cpp
│
├── tst_dependency/                 # L2: DependencyResolver, DependencyGraph, LevelCompatibilityChecker
│   ├── CMakeLists.txt
│   ├── tst_dependency_resolver.cpp
│   ├── tst_dependency_graph.cpp
│   ├── tst_level_checker.cpp
│   └── fixtures/
│       └── packages/               # mock package 目录（详见 §4）
│
├── tst_package/                    # L2: Package 解析、PackageManager
│   ├── CMakeLists.txt
│   ├── tst_package_parse.cpp
│   ├── tst_package_manager.cpp
│   └── fixtures/
│       └── packages/               # 各种合法/非法 package.json
│
├── tst_plugin/                     # L3: 插件加载、Level 校验、Task 生命周期
│   ├── CMakeLists.txt
│   ├── tst_plugin_loading.cpp
│   └── tst_task_lifecycle.cpp
│
├── tst_integration/                # L4: 端到端 G2p 管线（现有测试改造）
│   ├── CMakeLists.txt
│   ├── tst_g2p_pipeline.cpp
│   ├── TextSplitter.h / .cpp       # 沿用现有
│   ├── TextTagger.h / .cpp         # 沿用现有
│   └── configs/                    # 沿用现有 splitter/tagger configs
│
├── tst_dict/                       # L3: DsDict 插件测试
│   ├── CMakeLists.txt
│   ├── tst_dsdict.cpp
│   └── fixtures/
│       └── *.txt                   # 测试字典文件
│
└── tst_context/                    # L1/L2: Voice Bank Context 测试（已实现）
    ├── CMakeLists.txt
    ├── tst_fqid.cpp                # FQID 解析/格式化、context 名校验
    ├── tst_context_convert.cpp     # G2pInput/G2pRes context 字段验证
    ├── tst_context_isolation.cpp   # Context 隔离、跨 context 依赖失败、默认 context 回退
    ├── tst_context_dedup.cpp       # isSameMainModule 模块去重、selectBestModules
    ├── tst_context_version.cpp     # ContextKey、版本化 FQID、版本化 context 隔离/回退/去重
    ├── tst_framework.h             # 轻量级测试框架
    └── fixtures/
        └── packages/               # Mock packages（详见 §13）
```

### 2.3 CMake 集成模式

每个测试子目录的 `CMakeLists.txt` 遵循统一模式：

```cmake
project(tst_support)

file(GLOB _src *.cpp)
add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE
    LangCore::LangCore
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
```

### 2.4 路径参数化

消除所有硬编码绝对路径。通过 CMake 注入：

```cpp
// tests/common/TestConfig.h.in
#pragma once
#define TEST_PROJECT_ROOT "@PROJECT_ROOT_DIR@"
#define TEST_RES_DIR "@PROJECT_ROOT_DIR@/res"
#define TEST_PACKAGE_DIR "@PROJECT_ROOT_DIR@/res/G2pPackages"
```

各测试的 fixture 数据使用 `TEST_FIXTURE_DIR`（编译时定义为当前源目录下的 `fixtures/`）。

---

## 3. L1 单元测试：tst_support

### 3.1 tst_error.cpp

| 用例 | 验证内容 |
|------|---------|
| `defaultConstruct` | `Error()` → `ok() == true`, `type() == Success` |
| `typeConstruct` | `Error(ConfigError)` → `ok() == false`, 有默认消息 |
| `messageConstruct` | `Error(RuntimeError, "msg")` → `message() == "msg"` |
| `suggestionConstruct` | 带 suggestion → `hasSuggestion() == true` |
| `withContext` | 链式 `.withContext(file, line, func)` → `hasContext() == true`, `fullMessage()` 包含位置信息 |
| `withExtra` | `.withExtra("detail")` → `fullMessage()` 包含 extra |
| `successStatic` | `Error::success()` → `ok() == true` |
| `allTypes` | 遍历 0-10 所有 Type，验证 `ok()` 仅对 Success 为 true |

### 3.2 tst_expected.cpp

| 用例 | 验证内容 |
|------|---------|
| `valueConstruct` | `Expected<int>(42)` → `hasValue()`, `value() == 42`, `bool(exp) == true` |
| `errorConstruct` | `Expected<int>(Error(RuntimeError, "fail"))` → `!hasValue()`, `error().type() == RuntimeError` |
| `moveSemantics` | 移动构造后原对象不可用，新对象值正确 |
| `take` | `.take()` 取出值，语义上消耗 |
| `takeError` | `.takeError()` 取出错误 |
| `valueOr` | 有值时返回值，无值时返回 default |
| `voidSuccess` | `Expected<void>()` → `hasValue() == true` |
| `voidError` | `Expected<void>(Error(...))` → `!hasValue()` |
| `convertingMove` | `Expected<Derived>` → `Expected<Base>` 隐式转换 |
| `arrowOperator` | `exp->member` 访问 |
| `derefOperator` | `*exp` 解引用 |

### 3.3 tst_config_accessor.cpp

使用 fixture JSON 文件构造 `ConfigAccessor`。

**fixture: `valid_config.json`**
```json
{
  "name": "test",
  "count": 42,
  "rate": 3.14,
  "enabled": true,
  "tags": ["a", "b", "c"],
  "model_path": "models/test.onnx"
}
```

**fixture: `empty_config.json`**
```json
{}
```

| 用例 | 验证内容 |
|------|---------|
| `getString_present` | `getString("name")` → 成功，值 "test" |
| `getString_missing` | `getString("nonexist")` → Error(ConfigError) |
| `getString_default` | `getString("nonexist", "fallback")` → "fallback" |
| `getInt_present` | `getInt("count")` → 42 |
| `getInt_missing` | `getInt("nonexist")` → Error |
| `getDouble_present` | `getDouble("rate")` → 3.14 |
| `getBool_present` | `getBool("enabled")` → true |
| `getPath_present` | `getPath("model_path")` → 基于 basePath 的绝对路径 |
| `getStringArray_present` | `getStringArray("tags")` → {"a","b","c"} |
| `has_present` | `has("name")` → true |
| `has_absent` | `has("nonexist")` → false |
| `emptyConfig_allMissing` | 空 config 上所有 required getter → Error |
| `emptyConfig_defaults` | 空 config 上所有 optional getter → 返回默认值 |

### 3.4 tst_validation_chain.cpp（与 tst_config_accessor.cpp 合并或独立）

| 用例 | 验证内容 |
|------|---------|
| `emptyChain` | `ValidationChain().execute()` → 成功 |
| `singlePass` | `validateIntRange(5, 1, 10)` → 成功 |
| `singleFail` | `validateIntRange(15, 1, 10)` → Error(ValidationError) |
| `chainShortCircuit` | 第一项失败 → 不执行后续项，`error()` 为第一个错误 |
| `allPass` | 多项全通过 → 成功 |
| `stringAllowed` | `validateStringAllowed("fast", {"standard","fast"})` → 成功 |
| `stringNotAllowed` | `validateStringAllowed("turbo", {"standard","fast"})` → Error |
| `arrayNotEmpty` | 空 vector → Error；非空 → 成功 |
| `customValidator` | `.validate(lambda)` → lambda 返回值传播 |

---

## 4. L1 单元测试：tst_version

### 4.1 tst_version_range.cpp

#### 版本规范化

| 用例 | 输入 | 预期输出 |
|------|------|---------|
| `normalize_standard` | `"1.2.3"` | `"1.2.3"` |
| `normalize_pad` | `"1.2"` | `"1.2.0"` |
| `normalize_truncate` | `"1.2.3.4"` | `"1.2.3"` |
| `normalize_vPrefix` | `"v1.2.3"` | `"1.2.3"` |
| `normalize_prerelease` | `"1.2.3-beta"` | `"1.2.3"` |
| `normalize_single` | `"1"` | `"1.0.0"` |
| `normalize_empty` | `""` | `"0.0.0"` |

#### 版本比较

| 用例 | v1 / v2 | 预期 |
|------|---------|------|
| `compare_equal` | "1.2.3" / "1.2.3" | 0 |
| `compare_greater` | "2.0.0" / "1.9.9" | >0 |
| `compare_less` | "1.0.0" / "1.0.1" | <0 |
| `compare_majorDominates` | "2.0.0" / "1.99.99" | >0 |
| `compare_paddedEqual` | "1.2" / "1.2.0" | 0 |

#### 版本范围过滤（数据驱动）

使用 `QTest::addColumn` + `QTest::newRow` 的数据驱动模式：

| 范围字符串 | 可用版本 | 预期通过的版本 |
|-----------|---------|--------------|
| `"*"` | ["1.0.0","2.0.0"] | ["1.0.0","2.0.0"] |
| `""` | ["1.0.0"] | ["1.0.0"] |
| `"1.0.0"` | ["1.0.0","1.0.1"] | ["1.0.0"] |
| `">=1.0.0"` | ["0.9.0","1.0.0","1.1.0"] | ["1.0.0","1.1.0"] |
| `">1.0.0"` | ["1.0.0","1.0.1"] | ["1.0.1"] |
| `"<=2.0.0"` | ["1.0.0","2.0.0","3.0.0"] | ["1.0.0","2.0.0"] |
| `"<2.0.0"` | ["1.9.9","2.0.0"] | ["1.9.9"] |
| `"~1.2.0"` | ["1.2.0","1.2.5","1.3.0"] | ["1.2.0","1.2.5"] |
| `"1.0.0 - 2.0.0"` | ["0.9","1.0","1.5","2.0","2.1"] | ["1.0","1.5","2.0"] |
| `">=1.0 <2.0"` | ["0.9","1.0","1.5","2.0"] | ["1.0","1.5"]（多约束 AND） |

### 4.2 tst_version_resolver.cpp

构造 `vector<ModuleMetadata>` + `DependencyRequirement` 输入，验证 `VersionResolver::resolveDependency()` 结果。

| 用例 | 场景 | 预期 |
|------|------|------|
| `resolve_exactMatch` | 唯一候选完全匹配 | success, resolvedVersion 正确 |
| `resolve_highestVersion` | 多版本候选，"*" 范围 | 选最高版本 |
| `resolve_versionRange` | ">=1.0 <2.0" | 范围内最高版本 |
| `resolve_levelFilter` | level=1 + level=2 候选 | 仅匹配指定 level |
| `resolve_levelMinus1` | dep.level=-1 | 匹配请求方 level |
| `resolve_compatibleOp` | "~1.2.0" | 匹配 1.2.x |
| `fail_noCandidates` | 无匹配 packageId+moduleId | error "not found" |
| `fail_noVersionInRange` | 候选版本不在范围内 | error "No version in range" |
| `fail_noLevelMatch` | 版本匹配但 level 不匹配 | error "No version with required level" |
| `fail_emptyModules` | 空候选列表 | error |

---

## 5. L2 组件测试：tst_dependency

这是测试体系的核心，重点覆盖跨包依赖的各种场景。

### 5.1 Mock Package 体系

在 `tst_dependency/fixtures/packages/` 下创建 mock package 目录，每个测试场景一套。每个 mock package 仅包含 `package.json` 和必要的 `config.json`（无实际资源文件）。

#### 基础场景目录

```
fixtures/packages/
├── scenario-simple/               # 单包无依赖
│   └── pkg-alpha/
│       ├── package.json
│       └── modules/ModA/config.json
│
├── scenario-intra-dep/            # 包内依赖（A→B 同包）
│   └── pkg-beta/
│       ├── package.json
│       └── modules/
│           ├── ModA/config.json
│           └── ModB/config.json
│
├── scenario-cross-dep/            # 跨包依赖（PkgA→PkgB）
│   ├── pkg-provider/
│   │   ├── package.json
│   │   └── modules/ModBase/config.json
│   └── pkg-consumer/
│       ├── package.json
│       └── modules/ModDerived/config.json
│
├── scenario-diamond/              # 菱形依赖（A→B, A→C, B→D, C→D）
│   ├── pkg-d/                     # 被依赖的底层包
│   ├── pkg-b/
│   ├── pkg-c/
│   └── pkg-a/
│
├── scenario-chain/                # 链式跨包依赖（A→B→C→D，四个包）
│   ├── pkg-d/
│   ├── pkg-c/
│   ├── pkg-b/
│   └── pkg-a/
│
├── scenario-cycle-cross/          # 跨包循环依赖（PkgA→PkgB→PkgA）
│   ├── pkg-x/
│   └── pkg-y/
│
├── scenario-cycle-intra/          # 包内循环（A→B→A 同包）
│   └── pkg-cycle/
│
├── scenario-missing-dep/          # 缺失依赖（引用不存在的包）
│   └── pkg-orphan/
│
├── scenario-level-mismatch/       # Level 不兼容
│   ├── pkg-old/                   # level=1 的模块
│   └── pkg-new/                   # level=3 的模块依赖 level=1
│
├── scenario-version-conflict/     # 版本范围无法满足
│   ├── pkg-lib/                   # 提供 v1.0.0
│   └── pkg-app/                   # 要求 >=2.0.0
│
├── scenario-multi-provider/       # 多个包提供同 moduleId（不同 packageId）
│   ├── pkg-vendor-a/
│   └── pkg-vendor-b/
│
├── scenario-multi-version/        # 同包同模块多版本（selectBestModules 场景）
│   └── pkg-multi/
│
├── scenario-self-dep/             # 自依赖
│   └── pkg-self/
│
└── scenario-complex/              # 综合场景：6个包，混合跨包依赖+版本约束
    ├── pkg-core/
    ├── pkg-dict/
    ├── pkg-model-a/
    ├── pkg-model-b/
    ├── pkg-chain/
    └── pkg-app/
```

### 5.2 Mock Package 详细设计

#### scenario-cross-dep（跨包依赖基础案例）

**pkg-provider/package.json**:
```json
{
  "packageId": "provider",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "base-g2p",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/ModBase/config.json"
    }]
  }
}
```

**pkg-consumer/package.json**:
```json
{
  "packageId": "consumer",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "derived-g2p",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/ModDerived/config.json",
      "dependencies": [{
        "packageId": "provider",
        "moduleId": "base-g2p",
        "level": 1,
        "version": ">=1.0.0"
      }]
    }]
  }
}
```

#### scenario-cycle-cross（跨包循环）

**pkg-x/package.json**: moduleId "mod-x" 依赖 pkg-y 的 "mod-y"  
**pkg-y/package.json**: moduleId "mod-y" 依赖 pkg-x 的 "mod-x"

#### scenario-diamond（菱形依赖）

```
pkg-a (mod-a)
  ├── depends on pkg-b (mod-b)
  └── depends on pkg-c (mod-c)
       ├── mod-b depends on pkg-d (mod-d)
       └── mod-c depends on pkg-d (mod-d)
```

验证 mod-d 只初始化一次，拓扑排序为 d → b/c → a。

#### scenario-complex（综合场景）

```
pkg-core:     core-dict (dict, level=1, v2.0.0)
pkg-dict:     extra-dict (dict, level=1, v1.5.0), depends on pkg-core/core-dict >=1.0
pkg-model-a:  model-a (g2p, level=2, v1.0.0), depends on pkg-core/core-dict >=2.0
pkg-model-b:  model-b (g2p, level=2, v1.0.0), depends on pkg-core/core-dict ~2.0.0
pkg-chain:    chain-g2p (g2p, level=1, v1.0.0), depends on:
                  pkg-model-a/model-a (level=2, >=1.0)
                  pkg-dict/extra-dict (level=1, >=1.0)
pkg-app:      app-g2p (g2p, level=1, v1.0.0), depends on pkg-chain/chain-g2p >=1.0
```

验证：
- 完整依赖链解析正确
- 初始化顺序：core → dict → model-a → chain → app（model-b 独立）
- Level 混合场景（level=1 和 level=2 的模块共存）

### 5.3 tst_dependency_resolver.cpp

| 用例 | 场景 | 预期 |
|------|------|------|
| **基本功能** |||
| `resolve_noDeps` | scenario-simple | 所有模块标记为 resolved |
| `resolve_intraDep` | scenario-intra-dep | 包内依赖正确解析 |
| `resolve_crossDep` | scenario-cross-dep | 跨包依赖正确解析，resolvedDependencies 填充 |
| `resolve_diamond` | scenario-diamond | 菱形依赖正确解析，mod-d 仅出现一次 |
| `resolve_chain` | scenario-chain | 4 层链式依赖全部解析 |
| `resolve_complex` | scenario-complex | 综合场景全部解析成功 |
| **版本选择** |||
| `selectBest_highestVersion` | 同 pkg:module:level 有 v1.0 和 v2.0 | v2.0 保留，v1.0 被剔除 |
| `selectBest_differentLevels` | 同 module 不同 level | 各 level 独立保留 |
| **错误场景** |||
| `fail_missingDep` | scenario-missing-dep | resolvedModules 不含 orphan 模块，errors 非空 |
| `fail_selfDep` | scenario-self-dep | 自依赖模块被移除 |
| `fail_versionConflict` | scenario-version-conflict | 版本范围不满足，errors 包含 "No version in range" |
| `fail_levelMismatch` | scenario-level-mismatch | Level 不匹配，errors 包含 "No version with required level" |
| `fail_circularHint` | scenario-cycle-cross | 迭代无进展 → "Possible circular dependencies" |
| **边界** |||
| `empty_modules` | 空 vector | 无 crash，返回空结果 |
| `single_module_no_deps` | 1 个无依赖模块 | 直接 resolved |

### 5.4 tst_dependency_graph.cpp

| 用例 | 场景 | 预期 |
|------|------|------|
| **构图** |||
| `build_noDeps` | 无依赖模块 | buildGraph 成功，无边 |
| `build_linearChain` | A→B→C | buildGraph 成功 |
| `build_diamond` | scenario-diamond | buildGraph 成功 |
| `build_missingNode` | resolvedDep 指向不存在的节点 | buildGraph 返回 false |
| **环检测** |||
| `cycle_none` | 无环图 | findCycles 返回空 |
| `cycle_selfLoop` | A→A | 检测到环 |
| `cycle_twoNode` | A→B→A | 检测到 {A,B} |
| `cycle_crossPackage` | scenario-cycle-cross | 检测到跨包循环 |
| `cycle_inComplex` | 大图中仅部分节点成环 | 仅环成员返回，其余正常排序 |
| **拓扑排序** |||
| `topo_linear` | A→B→C | 顺序 [C, B, A] |
| `topo_diamond` | D←B←A, D←C←A | D 先于 B/C，B/C 先于 A |
| `topo_withCycle` | 含环 | 返回空 vector |
| **包级排序** |||
| `packageOrder_singlePkg` | 所有模块在同一包 | 1 个 PackageInitializationPlan |
| `packageOrder_crossPkg` | scenario-cross-dep | provider 先于 consumer |
| `packageOrder_diamond` | scenario-diamond | pkg-d 最先 |

### 5.5 tst_level_checker.cpp

| 用例 | pluginLevel / config | 预期 |
|------|---------------------|------|
| `compatible_exact` | P=2, min=1, max=3 | compatible |
| `compatible_atMin` | P=1, min=1, max=3 | compatible |
| `compatible_atMax` | P=3, min=1, max=3 | compatible |
| `incompatible_belowMin` | P=0, min=1, max=3 | incompatible, "below minimum" |
| `incompatible_aboveMax` | P=4, min=1, max=3 | incompatible, "exceeds maximum" |
| `maxZero_useCurrent` | P=2, min=1, max=0, current=2 | compatible (effectiveMax=2) |
| `maxZero_aboveCurrent` | P=3, min=1, max=0, current=2 | incompatible |
| `checkAll_mixed` | 多个 level | 返回所有检查结果 |
| `generateReport` | 混合结果 | 报告文本包含兼容/不兼容计数 |

---

## 6. L2 组件测试：tst_package

### 6.1 tst_package_parse.cpp

测试 `package.json` 解析的各种正常和异常情况。

**fixture packages/**:
```
valid-minimal/          # 仅 packageId
valid-full/             # 所有可选字段
valid-no-modules/       # 有 packageId 但无 modules 键
valid-localized/        # vendor/description 使用 i18n 格式
invalid-no-id/          # 缺少 packageId
invalid-modules-type/   # modules 不是 object
invalid-module-array/   # 模块数组不是 array
invalid-config-path/    # configuration 指向不存在的文件
invalid-chars-in-id/    # packageId 含禁用字符
```

| 用例 | fixture | 预期 |
|------|---------|------|
| `parse_minimal` | valid-minimal | 成功，version/vendor 为空 |
| `parse_full` | valid-full | 所有字段正确读取 |
| `parse_noModules` | valid-no-modules | 成功，模块列表为空 |
| `parse_localized` | valid-localized | DisplayText 正确解析 |
| `fail_noId` | invalid-no-id | Error(ConfigError) |
| `fail_modulesType` | invalid-modules-type | Error(ConfigError), 消息含 "modules" |
| `fail_moduleArray` | invalid-module-array | Error(ConfigError) |
| `fail_missingConfig` | invalid-config-path | 模块 level/version 使用 package 级别默认值或报错 |
| `fail_invalidChars` | invalid-chars-in-id | Error |

### 6.2 tst_package_manager.cpp

需要 `PackageManager` 实例（不是 `Manager` 单例）。测试包发现和模块元数据收集。

| 用例 | 验证内容 |
|------|---------|
| `addPackagePath_valid` | 添加合法路径，扫描后包含预期包 |
| `addPackagePath_invalid` | 添加不存在的路径，不崩溃 |
| `duplicateModule` | 同 mainModule 重复注册 → 只保留第一个 |
| `moduleMetadata_fields` | 验证 collectModuleMetadata 填充的每个字段 |
| `configOverride` | config.json 中的 `$version` 和 `level` 覆盖 package.json 值 |

---

## 7. L3 插件测试：tst_plugin

> 注意：L3 测试依赖插件 DLL 已构建。通过 CMake 条件（`if(TARGET plugin_target)`）控制是否编译。

### 7.1 tst_plugin_loading.cpp

| 用例 | 验证内容 |
|------|---------|
| `loadValidPlugin` | PluginFactory 加载已知 DLL，获取 Plugin 实例 |
| `pluginIid` | Plugin::iid() 返回正确值 |
| `pluginKey` | Plugin::key() 返回正确值 |
| `pluginApiLevel` | Plugin::apiLevel() 返回 > 0 |
| `pluginPath` | Plugin::path() 非空 |
| `loadMissingDll` | 不存在的路径 → 返回 nullptr |
| `pluginTypeSafe` | `plugin<TaskPlugin>(key)` → 正确类型 |

### 7.2 tst_task_lifecycle.cpp

| 用例 | 验证内容 |
|------|---------|
| `createTask` | TaskPlugin::createTask(spec) → 成功返回 Task |
| `initializeTask` | Task::initialize() → Expected<void> 成功 |
| `taskApiLevel` | Task::apiLevel() > 0 |
| `taskSpec` | Task::spec() == 传入的 ModuleSpec |
| `startTask` | Task::start(input) → Expected<TaskResult> |
| `getConfig` | Task::getConfig() → 非空 JSON |

---

## 8. L3 字典插件测试：tst_dict

**fixture: `test_dict.txt`**:
```
hello	hh ah l ow
world	w er l d
```

| 用例 | 验证内容 |
|------|---------|
| `lookup_found` | key="hello" → found=true, values=["hh ah l ow"] |
| `lookup_notFound` | key="missing" → found=false |
| `lookup_withDefault` | key="missing" + default="??" → value="??", 但 found 语义正确 |
| `lookup_multiple` | keys=["hello","world"] → foundCount=2 |
| `lookup_mixed` | keys=["hello","missing"] → foundCount=1 |
| `emptyKeys` | keys=[] → found=true, foundCount=0, values=[] |

---

## 9. L4 端到端测试：tst_integration

改造现有 `tst_langCore/main.cpp` 为 Qt Test 类。

### 9.1 tst_g2p_pipeline.cpp

```cpp
class TestG2pPipeline : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();      // Manager::initialize, splitter/tagger init
    void cleanupTestCase();

    void configApi();
    void convertMixed();
    void edgeCase_empty();
    void edgeCase_specialChars();
    void edgeCase_mixedBoundaries();
    void edgeCase_singleChar();
    void edgeCase_numbersAndSymbols();
    void performance();       // QBENCHMARK
};
```

| 用例 | 验证内容 |
|------|---------|
| `configApi` | `getConfig()` 返回非空字符串，长度 > 0 |
| `convertMixed` | 中英日混合文本 → 每个 G2pRes 的 pronunciation 非空（mode="convert" 时） |
| `edgeCase_empty` | 空输入 → 空结果，无崩溃 |
| `edgeCase_specialChars` | `"!@#$%"` → 结果数量 > 0，errorType == NoError |
| `edgeCase_mixedBoundaries` | `"a中b日c"` → 正确分段 |
| `edgeCase_singleChar` | `"中"` → 1 个结果 |
| `performance` | 100 个随机单词 → `QBENCHMARK` 度量时间 |

### 9.2 跳过条件

若 ONNX Runtime 不可用（OnnxDriver 初始化失败），使用 `QSKIP("OnnxDriver not available")` 跳过依赖 ONNX 的用例，而非整个测试。

---

## 10. 跨包依赖专项测试矩阵

本节汇总所有与跨包依赖相关的测试用例，确保覆盖每种可能的依赖拓扑和错误模式。

### 10.1 正常拓扑

| ID | 拓扑 | 包数 | 验证重点 |
|----|------|------|---------|
| D-01 | 无依赖 | 1 | 独立包正常加载 |
| D-02 | A→B（包内） | 1 | 包内模块间依赖，initOrder 中 B 先于 A |
| D-03 | PkgA→PkgB | 2 | 跨包依赖，PkgB 先初始化 |
| D-04 | A→B→C（跨包链） | 3 | 链式传递，C→B→A 顺序 |
| D-05 | A→B→C→D（4 层链） | 4 | 长链解析（测试迭代轮次） |
| D-06 | 菱形：A→{B,C}→D | 4 | D 只出现一次，B/C 可任意顺序 |
| D-07 | 混合包内+跨包 | 3 | PkgA 内 A1→A2，A2→PkgB.B1 |
| D-08 | 综合 6 包 | 6 | scenario-complex，多层次混合 |

### 10.2 版本约束

| ID | 场景 | 预期 |
|----|------|------|
| V-01 | dep 要求 `>=1.0`，提供 v1.0, v2.0 | 解析到 v2.0 |
| V-02 | dep 要求 `~1.2.0`，提供 v1.2.0, v1.2.5, v1.3.0 | 解析到 v1.2.5 |
| V-03 | dep 要求 `>=2.0`，提供 v1.0 | 失败："No version in range" |
| V-04 | dep 要求 `1.0.0 - 2.0.0`，提供 v1.5, v2.5 | 解析到 v2.0（若有）或 v1.5 |
| V-05 | dep 要求 `*`，提供多版本 | 选最高版本 |
| V-06 | 多约束 `>=1.0 <2.0` | AND 语义 |

### 10.3 Level 约束

| ID | 场景 | 预期 |
|----|------|------|
| L-01 | dep 要求 level=1，候选 level=1 | 成功 |
| L-02 | dep 要求 level=2，候选仅 level=1 | 失败 |
| L-03 | dep 要求 level=-1，候选 level 与请求方相同 | 成功（隐式匹配） |
| L-04 | dep 要求 level=-1，候选 level 与请求方不同 | 失败 |
| L-05 | 系统 level 检查：pluginLevel 在 [min,max] 范围 | 通过 |
| L-06 | 系统 level 检查：pluginLevel < min | 拒绝加载 |
| L-07 | 系统 level 检查：pluginLevel > max | 拒绝加载 |

### 10.4 错误场景

| ID | 场景 | 预期 |
|----|------|------|
| E-01 | 依赖的 packageId 不存在 | 错误含 "Dependency not found" |
| E-02 | 依赖的 moduleId 不存在 | 错误含 "Dependency not found" |
| E-03 | 跨包循环 PkgA↔PkgB | findCycles 检测到，topo 返回空 |
| E-04 | 包内循环 A↔B | 同上 |
| E-05 | 三节点环 A→B→C→A | 同上 |
| E-06 | 自依赖 | 模块被移除 |
| E-07 | 环+非环混合图 | 非环部分正常排序，环部分标记为 cycle |
| E-08 | 菱形中一条边版本不满足 | 部分解析失败，受影响的上游模块也失败 |
| E-09 | 传递依赖缺失（A→B→C，C 不存在） | A 和 B 都无法解析 |
| E-10 | 两个包提供同 moduleId（不同 packageId） | 依赖声明指定 packageId 后精确匹配 |

---

## 11. 实施优先级

### 第一批（核心单元测试，无外部依赖）

1. `tst_support`（Error, Expected, ConfigAccessor, ValidationChain）
2. `tst_version`（VersionRange, VersionResolver）
3. `tst_dependency`（DependencyResolver, DependencyGraph, LevelChecker）

**理由**：这三个模块是框架基础，且完全可以用 mock 数据测试，不需要插件 DLL 或外部库。覆盖 §10 矩阵中的全部 D/V/L/E 用例。

### 第二批（包管理和解析）

4. `tst_package`（Package 解析，PackageManager 元数据收集）

**理由**：依赖第一批的 mock package fixtures，验证从磁盘到内存的解析链路。

### 第三批（需要构建插件）

5. `tst_plugin`（插件加载，Task 生命周期）
6. `tst_dict`（DsDict 插件）
7. `tst_integration`（端到端 G2p 管线）

**理由**：需要 `LANGMGR_BUILD_PLUGINS=ON`，依赖外部库（ONNX Runtime, cpp-pinyin）。CTest 通过标签或条件编译控制。

---

## 12. Review 指南

### 12.1 测试 review 检查清单

- [ ] 每个测试用例是否有明确的 `QCOMPARE` 或 `QVERIFY` 断言？
- [ ] 测试用例名是否清晰描述场景和预期？
- [ ] 错误路径测试是否验证了错误类型和错误消息的关键内容？
- [ ] fixture 数据是否自包含（不依赖外部资源）？
- [ ] 是否有遗漏的边界条件？
- [ ] 数据驱动测试是否覆盖了充分的输入组合？

### 12.2 模块 review 顺序建议

```
tst_support  → 最基础，应首先 review 并合入
    ↓
tst_version  → 依赖 Expected/Error 的正确性
    ↓
tst_dependency → 依赖 VersionResolver 的正确性
    ↓
tst_package  → 依赖 dependency 子系统的正确性
    ↓
tst_plugin / tst_dict / tst_integration → 依赖全部核心
```

每个 PR 应仅包含一个 `tst_*` 目录，便于聚焦 review。

---

## 12.3 PRD §14 回归测试清单

以下测试用例用于验证 PRD §14 中记录的 bug 修复和设计问题，防止回归。应分散到对应的测试模块中。

### 已修复项 — 回归防护

| PRD § | 分类 | 建议测试位置 | 测试内容 |
|-------|------|------------|---------|
| 14.10 | LstmG2p g2pId | `tst_plugin` 或 `tst_integration` | 验证 `G2pRes.g2pId` 等于模块 `spec->id()`，而非硬编码值 |
| 14.16 | pluginsDirty | `tst_plugin` | 调用 `plugin()` 两次，验证第二次不触发重复目录扫描 |

### 待修复项 — 验证修复后的行为

| PRD § | 分类 | 建议测试位置 | 测试内容 |
|-------|------|------------|---------|
| 14.11 | MandarinG2p copy | `tst_integration` | "copy" 模式词的 `G2pRes.lyric` 应与输入完全一致 |
| 14.13 | FormatStep 命名 | `tst_unit` | 边界输入（`"AH0L"`, `"AH0 L OW1"`, 混合 alphanumeric/symbol）的输出验证 |
| 14.14 | checkDependencies | `tst_dependency` | 准备多个不兼容模块，验证返回的错误列表包含所有不兼容项 |
| 14.15 | Expected\<T\> 默认构造 | `tst_support` | 编译期验证：对不可默认构造类型使用 `Expected<T>` 应编译失败或受 SFINAE 约束 |
| 14.17 | Session::close | `tst_plugin` | Session open→close 生命周期，特别是 path_map 命中路径 |
| 14.18 | selectBestModules | `tst_dependency` | 3+ 个同 module 不同版本，验证保留最高版本且无崩溃 |
| 14.19 | Task::Mgr() | `tst_support` | 默认构造 Task 调用 `Mgr()` 应返回 nullptr 或 assert，不崩溃 |
| 14.20 | VersionedTaskManager | `tst_support` | 未调用 `setImpl()` 的 VersionedTaskManager 调用 `initialize()` 应安全失败 |
| 14.22 | dependencyGraph | `tst_dependency` | 连续两次调用 `checkDependencies()`，验证第二次结果正确 |
| 14.24 | Mandarin/Cantonese init | `tst_plugin` | 底层库初始化失败时 `Task::initialize()` 应返回错误 |
| 14.25 | LstmG2p .take() | `tst_plugin` | 缺少 tensor 时 `start()` 应返回错误而非崩溃 |
| 14.29 | Parser_impl.h | `tst_unit` | `parse_stringVec_required` 多元素输入，验证 `out` 包含所有解析结果 |

---

## 13. Voice Bank Context 测试（tst_context）— 已实现

> 对应设计文档：`docs/VoiceBank-Scoped-Package-Design.md`

### 13.1 测试目标

验证 Context 隔离、去重、回退、FQID 解析、ContextKey 版本化等机制的正确性。这些测试均为 L1/L2 级别，不依赖插件 DLL 或外部库。

### 13.2 目录结构（已实现）

```
tst_context/
├── CMakeLists.txt
├── main.cpp                        # Catch2 main (CATCH_CONFIG_MAIN)
├── tst_fqid.cpp                    # FQID 解析/格式化、context 名校验
├── tst_context_convert.cpp         # G2pInput/G2pRes context 字段验证
├── tst_context_isolation.cpp       # Context 隔离、跨 context 依赖失败、默认 context 回退
├── tst_context_dedup.cpp           # isSameMainModule 模块去重、selectBestModules
└── tst_context_version.cpp         # ContextKey、版本化 FQID、版本化 context 隔离/回退/去重
```

### 13.3 tst_fqid.cpp — FQID 解析/格式化与 Context 名校验

| 用例 | 输入 | 预期 context | 预期 moduleId |
|------|------|------------|-------------|
| `parse_plain` | `"g2p-cmn-official"` | `""` | `"g2p-cmn-official"` |
| `parse_withContext` | `"SingerA:g2p-cmn-custom"` | `"SingerA"` | `"g2p-cmn-custom"` |
| `parse_emptyContext` | `":g2p-cmn"` | `""` | `"g2p-cmn"` |
| `parse_multipleColons` | `"A:B:C"` | `"A"` | `"B:C"`（首个 `:` 分隔） |
| `format_plain` | context=`""`, id=`"g2p-cmn"` | FQID = `"g2p-cmn"` |
| `format_withContext` | context=`"SingerA"`, id=`"g2p-cmn"` | FQID = `"SingerA:g2p-cmn"` |
| `contextName_valid` | `"SingerA"`, `"singer_01"`, `"a.b-c"` | 校验通过 |
| `contextName_invalid` | 含空格、`:`、`/`、超长等 | 校验失败 |

### 13.4 tst_context_convert.cpp — G2pInput/G2pRes Context 验证

| 用例 | 场景 | 预期 |
|------|------|------|
| `g2pInput_defaultContext` | G2pInput{"hello", "eng-cmu", ""} | context 为空，contextVersion 为 null |
| `g2pInput_withContext` | G2pInput{"你好", "g2p-cmn-custom", "SingerA"} | context 和 g2pId 正确 |
| `g2pInput_withVersion` | G2pInput 携带 contextVersion | contextVersion 字段正确传播 |
| `g2pRes_contextField` | G2pRes 包含 context + contextVersion | 字段与输入一致 |

### 13.5 tst_context_isolation.cpp — Context 隔离与回退

使用 mock ModuleMetadata 列表调用 DependencyResolver。

| 用例 | 场景 | 预期 |
|------|------|------|
| `isolate_sameModuleId` | SingerA 和 SingerB 都有 "g2p-cmn-custom" | 两者都解析成功，互不干扰 |
| `isolate_noCrossDep` | SingerA 的模块依赖声明指向 SingerB 的 packageId | 解析失败（跨 context 不可见） |
| `fallback_toDefault` | SingerC 的模块依赖 "cmn-official:g2p-cmn-official"，仅默认 context 提供 | 回退成功 |
| `fallback_preferLocal` | SingerA context 和默认 context 都有 "g2p-cmn-x"，SingerA 模块依赖它 | 优先选 SingerA context 内的 |
| `noFallback_crossContext` | SingerA 模块依赖 SingerB 的 packageId，默认 context 也无 | 失败 |
| `defaultContext_globalVisibility` | 默认 context 的模块可被所有 context 依赖 | 通过 |
| `otherContext_notVisibleToDefault` | 默认 context 的模块依赖 SingerA 的 packageId | 失败（反向不可见） |

### 13.6 tst_context_dedup.cpp — 模块去重（isSameMainModule / selectBestModules）

| 用例 | 场景 | 预期 |
|------|------|------|
| `dedup_sameQuad` | SingerA v1 和 v2 携带相同 (moduleId, iid, level, version) | 只加载一份，日志含 "already loaded...skipping" |
| `dedup_diffVersion` | SingerA v1 携带 v1.0，v2 携带 v2.0 | 两者都加载，selectBestModules 保留 v2.0 |
| `dedup_diffIid` | 同 moduleId+version 但不同 iid（plugin class） | 不去重，两者都加载 |
| `dedup_diffLevel` | 同 moduleId+version+iid 但不同 level | 不去重，两者都加载 |
| `dedup_crossContext_noDedup` | SingerA 和 SingerB 都有相同四元组 | 不去重，各自独立加载 |
| `dedup_defaultContext` | 默认 context 内重复四元组 | 按同样规则去重 |

### 13.7 tst_context_version.cpp — ContextKey 版本化

| 用例 | 场景 | 预期 |
|------|------|------|
| `contextKey_basic` | ContextKey 构造、比较、isDefault/isVersioned | 值语义正确 |
| `contextKey_toString` | 默认 → "(default)"，带版本 → "SingerA@2.0.0" | 格式正确 |
| `versionedFqid_format` | formatFqid({"SingerA", 2.0.0}, "g2p-cmn") | "SingerA@2.0.0:g2p-cmn" |
| `versionedFqid_parse` | parseFqid("SingerA@2.0.0:g2p-cmn") | context="SingerA", version=2.0.0, moduleId="g2p-cmn" |
| `versionedIsolation` | 同 context 不同版本的模块隔离 | 各版本独立，互不干扰 |
| `versionedFallback` | 带版本查找失败，退化到无版本 | 退化成功 |
| `versionedDedup` | 同 ContextKey 内 isSameMainModule | contextVersion 参与判定 |

### 13.8 Mock Package Fixtures

#### fixtures/packages/official/pkg-cmn/package.json
```json
{
  "packageId": "cmn-official",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "g2p-cmn-official",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/G2p/config.json"
    }]
  }
}
```

#### fixtures/packages/singerA-v1/pkg-custom/package.json
```json
{
  "packageId": "singerA-custom",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "g2p-cmn-custom",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/G2p/config.json"
    }]
  }
}
```

#### fixtures/packages/singerA-v2-same/pkg-custom/package.json
同上（packageId、moduleId、version 均相同——用于测试去重）

#### fixtures/packages/singerB/pkg-custom/package.json
```json
{
  "packageId": "singerB-custom",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "g2p-cmn-custom",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/G2p/config.json"
    }]
  }
}
```

注意：singerA 和 singerB 的 `moduleId` 相同（`g2p-cmn-custom`），但 `packageId` 不同。在各自 context 内不冲突。

#### fixtures/packages/singerC-dep-official/pkg-custom/package.json
```json
{
  "packageId": "singerC-custom",
  "version": "1.0.0",
  "modules": {
    "g2p": [{
      "moduleId": "g2p-cmn-enhanced",
      "class": "g2p.mock.MockG2p",
      "level": 1,
      "configuration": "modules/G2p/config.json",
      "dependencies": [{
        "packageId": "cmn-official",
        "moduleId": "g2p-cmn-official",
        "level": 1,
        "version": ">=1.0.0"
      }]
    }]
  }
}
```

---

**文档版本**: 2.0  
**最后更新**: 2026-04-27