# 设计准则与决策

> 本文档是 Language Manager 项目的设计准则统一目录，将散落在 PRD、Conventions-and-Standards、Issues-Tracker 中的原则整合到统一编号体系中。
>
> 编写格式参考 dataset-tools 的 [human-decisions.md](file:///D:/projects/dataset-tools/docs/human-decisions.md) 分层分类体系。

---

## 第一章：ARCH — 架构与模块设计

### ARCH-01：插件职责单一

- **来源**：PRD §1.1「简洁可靠」、Conventions §9
- **理由**：每个插件只做一件事，减少内部状态机复杂度。遇错直接返回，不设计重试或回滚逻辑。
- **禁止模式**：插件内部实现状态重试、自动回滚、或"尽力而为"的 fallback 链（正确的 fallback 应在 ChainG2p 流水线层面协调，而非单插件内部）。
- **正确做法**：插件 `start()` 返回值清晰表达成功/失败。失败由上层（如 ChainG2p 的 FallbackStep）统一处理。
- **关联**：ROBUST-01、ARCH-02

### ARCH-02：接口稳定，Level 锚定兼容性

- **来源**：PRD §1.1「接口稳定」、PRD §2.1、Conventions §9
- **理由**：公共头文件即契约。Level（整数）锚定 API 结构兼容性；Version（semver）描述同一 Level 下的实现差异。依赖方通过版本范围约束（`>=1.0`, `~2.1`, `1.0-2.0`, `*`）选择实现版本。
- **禁止模式**：在不递增 Level 的情况下修改公共头文件的函数签名或结构体布局。
- **正确做法**：任何破坏性 API 变更必须同步递增 Level。新实现可注册到同一 Level（若兼容）或更高 Level。推荐 Version 首位与 Level 一致（Level=1 → Version=1.x.x）。
- **关联**：INFRA-01

### ARCH-03：组合优于继承

- **来源**：VersionedTaskManager 设计实践、PRD §14.7
- **理由**：`Manager` → `PackageManager` → `PluginFactory` 三层 public 继承虽然存在，但核心功能通过 `VersionedTaskManager` 的组合模式（持有 `VersionedTaskImplBase` 子类指针）实现，避免了深层继承的脆弱性。
- **禁止模式**：新增功能时不要继续加深继承链。不要在基类中存放状态后被子类遮蔽（如 PRD §14.1 的成员遮蔽问题）。
- **正确做法**：优先使用组合。新插件通过 `VersionedTaskImplBase` 接口 + `VersionedTaskManager` 组合实现多版本支持。
- **关联**：ARCH-01

### ARCH-04：相似模块统一设计

- **来源**：REF-OP-01、REF-OP-02、PRD §14.11/§14.12
- **理由**：两个类有 >60% 相同代码时，应提取公共实现。本项目 MandarinG2p 和 CantoneseG2p 曾因独立维护导致 bug 不对称（mode 分类、getConfig 缓存曾在修复时遗漏其中一个插件）。统一后可杜绝不对称 bug。
- **禁止模式**：复制粘贴创建新插件。独立维护两个高度相似的模块。
- **正确做法**：提取公共基类或共享库。差异点通过模板参数、构造函数参数或虚方法注入。
- **关联**：ROBUST-03

### ARCH-05：结构化控制流

- **来源**：REF-OP-03、PRD §14.17、Issues-Tracker §1.3
- **理由**：goto 标签使得控制流的正确性依赖对多级索引的心智模型，后续修改极易引入回归 bug。本项目 Session::open/close 在重构前有 4 个 goto 标签。
- **禁止模式**：在应用层代码中使用 goto 进行控制流跳转（RAII 资源清理的 `goto cleanup` 模式除外）。
- **正确做法**：用 if-else 链、提前 return、子函数提取替代 goto。
- **关联**：ARCH-01

---

## 第二章：ROBUST — 健壮性与错误处理

### ROBUST-01：Expected\<T\> 传播错误

- **来源**：PRD §1.1、Conventions §3、AGENTS.md
- **理由**：`Expected<T>` 提供类型安全的错误传播，强制调用方处理错误路径。通过 `.take()` / `.takeError()` 显式分离成功和失败分支。
- **禁止模式**：应用层逻辑抛出异常。忽略 `Expected` 的返回值。在未检查 `Expected` 状态的情况下调用 `.take()`。
- **正确做法**：所有可能失败的函数返回 `Expected<T>`。调用方先检查状态再访问值。`Expected<void>` 用于纯副作用操作。
- **关联**：ROBUST-02、ROBUST-03

### ROBUST-02：异常边界隔离

- **来源**：PRD §5.4、AGENTS.md
- **理由**：try-catch 仅用于第三方库边界（JSON 解析、ONNX Runtime、cpp-pinyin、std::regex 等），将外部异常转换为 `Error`。这使得异常传播范围可控，不会穿越模块边界。
- **禁止模式**：在应用层逻辑中使用 try-catch。让第三方库异常逃逸到调用方。
- **正确做法**：在每个第三方库调用点用 try-catch 包裹，将所有异常类型（`std::exception`、`Ort::Exception` 等）转为 `Error`。每个 catch 必须记录日志或返回错误。
- **关联**：ROBUST-01、ROBUST-03

### ROBUST-03：catch 禁止静默吞掉异常

- **来源**：PRD §14.6、PRD §14.26、AGENTS.md
- **理由**：静默吞掉的异常将导致 bug 不可观测。PRD §14.6 曾修复 `VersionUtils.cpp` 中 `catch(...)` 静默吞掉异常的问题。
- **禁止模式**：`catch(...)` 不记录日志也不返回错误。空 catch 块。
- **正确做法**：每个 catch 块必须至少记录一条日志（`Log.langCoreWarning(...)`）或返回 `Error`。对于可恢复的异常（如单个词的拼音转换失败），记录日志并回退到 copy 模式。
- **关联**：ROBUST-02

### ROBUST-04：空指针防御性检查

- **来源**：PRD §14.19、PRD §14.20
- **理由**：插件由动态加载的 DLL 提供，`spec` 和 `_impl` 可能为空。PRD §14.19 和 §14.20 曾修复 `Task::Mgr()` 和 `VersionedTaskManager` 中的空指针解引用问题。
- **禁止模式**：直接解引用指针而不检查是否为 nullptr。假设外部传入的指针一定非空。
- **正确做法**：对 `spec`、`_impl`、`input` 等外部传入或间接持有的指针，在使用前检查 nullptr。空指针返回 `Error(NullPointerError)`。
- **关联**：ROBUST-01

### ROBUST-05：容器元素迭代器失效防护

- **来源**：PRD §14.18
- **理由**：PRD §14.18 曾修复 `DependencyResolver::selectBestModules` 中 `remove_if` 移动元素后原始指针失效的问题。
- **禁止模式**：在修改容器后继续使用之前获取的指针或引用。在 `remove_if`/`erase` 过程中使用未更新的迭代器。
- **正确做法**：改用 index + key 比较替代原始指针。使用 `erase-remove` 惯用法后不依赖之前的迭代器。
- **关联**：ARCH-01

---

## 第三章：INFRA — 基础设施与配置

### INFRA-01：ConfigAccessor Required/Optional 分离

- **来源**：Conventions §5、AGENTS.md
- **理由**：`cfg.getString("key")`（required）在字段缺失时返回错误；`cfg.getString("key", "default")`（optional）提供默认值。调用方可根据字段的重要性选择语义。
- **禁止模式**：使用 `try-catch` 访问配置。在 `initialize()` 中动态修改配置后不缓存。
- **正确做法**：严格匹配方法语义——必须存在的字段用 Required 版本，可选字段用 Optional 版本。在 `initialize()` 中一次性读取所有配置并缓存。
- **关联**：ARCH-02

### INFRA-02：日志分类约定

- **来源**：Conventions §6、AGENTS.md
- **理由**：每个模块声明独立的 `LogCategory`，日志按级别分类（info/warning/debug/error）。格式使用 `%1`, `%2` 占位符（Qt 风格），或使用 `langCoreInfoF` 等 printf 风格变体。
- **禁止模式**：使用 `std::cout`/`std::cerr` 直接输出。日志占位符与实际参数数量不匹配。
- **正确做法**：声明 `LogCategory`，使用 `Log.langCoreInfo(...)` 等宏。日志信息包含足够上下文（模块名、文件名、关键参数值）。
- **关联**：ROBUST-03

### INFRA-03：插件注册规范

- **来源**：Plugin-Development-Guide.md
- **理由**：Task 插件使用 `LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, "category.key", ApiLevel)` 注册；Driver 插件使用 `LANGCORE_DEFINE_DRIVER_PLUGIN(...)`。这些宏内部调用 `LANGCORE_EXPORT_PLUGIN`。
- **禁止模式**：手动拼写导出符号。注册 key 与其他插件冲突。
- **正确做法**：使用提供的注册宏。key 格式为 `"category.plugin-name"`。`ApiLevel` 从 `spec->apiLevel()` 获取。
- **关联**：ARCH-02

### INFRA-04：CMake 项目命名约定

- **来源**：Conventions §8
- **理由**：`project()` 名称与插件注册 key 一致。`LangPlugins_add_plugin()` / `LangPlugins_add_library()` 封装了编译选项。构建输出目录由构建系统统一管理。
- **禁止模式**：插件 project 名称与注册 key 不一致。手动设置目标输出路径。
- **正确做法**：使用统一 CMake 函数。测试目录从 `tests/` 根级 `add_subdirectory()` 注册。`target_include_directories` 设置共享头文件路径。
- **关联**：ARCH-04

### INFRA-05：测试分级管理

- **来源**：Test-Design-Document.md
- **理由**：测试分为三级：L1（纯逻辑，无插件 DLL）、L2（需要插件 DLL）、L3（端到端集成）。共享的测试基础设施（框架头文件、mock fixtures）提取到 `tests/common/`。
- **禁止模式**：在不同测试目录中维护相同的测试框架头文件。将 mock 数据硬编码在测试用例中。
- **正确做法**：共享代码放入 `tests/common/`。L1 测试不依赖插件 DLL。每个测试目录有独立的 `CMakeLists.txt`。
- **关联**：ARCH-04

---

## 第四章：PACK — 包管理准则

### PACK-01：依赖显式声明与静态校验

- **来源**：PRD §2.1、Architecture-Overview.md
- **理由**：插件间的依赖通过 `package.json` 的 `dependencies` 字段显式声明，`DependencyResolver` 在加载时静态校验，CMake 通过 `target_link_libraries` 编译期 enforce。三重保障机制比运行时的动态依赖注入更可靠。
- **禁止模式**：隐式假设某个插件一定存在而不声明依赖。
- **正确做法**：所有模块间依赖在 `package.json` 中显式声明。`DependencyResolver` 自动检查 Level 兼容性和版本范围。
- **关联**：ARCH-02、INFRA-04

### PACK-02：依赖解析防循环

- **来源**：PRD §14.3
- **理由**：依赖图循环会导致无限递归加载。项目使用 Kahn 拓扑排序检测环——排序完成后未被访问的节点即为环成员。
- **禁止模式**：在依赖声明中形成循环引用（A 依赖 B，B 依赖 A）。
- **正确做法**：若两个模块相互需要，提取公共接口到第三个模块。`DependencyGraph::findCycles()` 在加载前检测并报告环。
- **关联**：PACK-01

---

## 附录A：已废止决策

| 编号 | 决策 | 废止原因 | 替代方案 |
|------|------|---------|---------|
| — | 暂无已废止决策 | — | — |

---

## 附录B：ADR 冲突解决记录

| 日期 | 议题 | 冲突选项 | 最终决策 | 理由 |
|------|------|---------|---------|------|
| 2026-05-19 | REF-OP-01 基类设计 | 模板参数 vs 虚方法注入 | 虚方法注入 | 模板需将实现暴露在头文件中，不适合插件 DLL 场景；虚方法让子类简洁（~20 行） |
| 2026-05-19 | REF-OP-03 goto 重构 | 子函数提取 vs 内联 if-else | 内联 if-else | Session 是热路径，额外函数调用增加开销；用 `foundExisting` 标志 + if-else 链可替代所有 goto 标签 |

---

**文档版本**: 1.0  
**关联文档**: [PRD-v2.0.md](file:///d:/projects/language-manager/docs/PRD-v2.0.md) · [Conventions-and-Standards.md](file:///d:/projects/language-manager/docs/Conventions-and-Standards.md) · [Issues-Tracker.md](file:///d:/projects/language-manager/docs/Issues-Tracker.md) · [Test-Design-Document.md](file:///d:/projects/language-manager/docs/Test-Design-Document.md) · [host-integration/README.md](file:///d:/projects/language-manager/docs/host-integration/README.md)