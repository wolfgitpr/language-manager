# Language Manager 设计审查报告

## 审查目标

分析 docs/ 文档与现有代码的设计完善性，聚焦：

* 简洁可靠、接口稳定抽象

* 插件系统特性：core 更新稳定，部分插件长期不更新

* 免维护：不能崩溃或不兼容

***

## 一、严重问题（崩溃/不兼容风险）

### 1.1 C++ ABI 跨 DLL 边界无保护

**现状**：插件系统唯一的 C ABI 边界是 `langCore_plugin_instance()` 导出函数。获取 `Plugin*` 后，所有交互通过 C++ 虚函数表、`std::shared_ptr`、`std::string`、`Expected<T>` 等 C++ 类型直接跨 DLL 传递。

**风险**：

* 虚函数表布局依赖编译器版本和编译选项

* `std::shared_ptr`、`std::string` 等 STL 类型布局依赖 C++ 运行时版本

* `Expected<T>` 作为值类型跨 DLL 返回，布局依赖编译器

* 核心/插件使用不同编译器版本 → 虚表偏移错位 → 崩溃

* 核心/插件链接不同版本 STL → `shared_ptr` 引用计数布局不同 → 内存错误

**影响**：长期不更新的插件如果用旧版编译器构建，新版 core 升级编译器后即不兼容。违反"免维护不能崩溃"要求。

**建议**：

* 方案 A（推荐）：在 `LANGCORE_EXPORT_PLUGIN` 中嵌入 ABI 标记（编译器版本、STL 版本哈希），加载时校验

* 方案 B：定义稳定的 C 结构体接口层，插件侧 C++ 包装

* 方案 C：要求所有插件与 core 使用完全相同的工具链构建（当前隐式约定，需文档化并强制校验）

### 1.2 `NO<T>::as()` 使用 `static_pointer_cast` 无类型安全

**现状**：[NamedObject.h:83-86](file:///d:/projects/language-manager/core/include/LangCore/Base/NamedObject.h#L83-L86)

```cpp
template <class U>
NO<U> as() const noexcept {
    return std::static_pointer_cast<U>(*this);
}
```

**风险**：

* `static_pointer_cast` 不做 RTTI 校验，类型不匹配时返回悬空指针（非 nullptr）

* `Manager::convert()` 中 `if (const auto g2pRes = _result.as<G2pResultV1>())` 只检查空指针，无法检测类型不匹配

* 如果 core 升级引入 `G2pResultV2`，旧插件的 `as<G2pResultV1>()` 不会返回 nullptr，而是指向错误内存 → UB

**影响**：Level 升级引入新类型时，旧插件无法安全检测类型不匹配，直接崩溃。

**建议**：

* 在 `NamedObject` 基类添加 `virtual const std::type_info &typeInfo() const { return typeid(*this); }`

* `as<U>()` 先校验 `typeInfo() == typeid(U)`，不匹配返回空 `NO<U>`

* 或提供 `tryAs<U>()` 方法做安全转型，`as<U>()` 保留为高性能路径但加 debug assert

### 1.3 插件加载无崩溃隔离

**现状**：[PluginFactory.cpp](file:///d:/projects/language-manager/core/lib/Core/PluginFactory.cpp) 中调用 `getter()` 获取插件实例时无任何异常保护：

```cpp
const auto getter = reinterpret_cast<PluginGetter>(so.resolve("langCore_plugin_instance"));
if (auto plugin = getter(); ...) {  // ← 无 try-catch / SEH
```

**风险**：

* 插件 DLL 构造函数崩溃 → 整个进程崩溃

* `reinterpret_cast` 强转导出函数签名，签名不匹配 → 栈损坏

* 恶意/损坏的 DLL → 任意崩溃

**影响**：一个有问题的插件 DLL 可以让整个系统崩溃，违反"免维护不能崩溃"。

**建议**：

* Windows 平台使用 `__try/__except` (SEH) 包裹 `getter()` 调用

* 跨平台使用 `setjmp/longjmp` 或信号处理

* 捕获异常后记录错误并 `continue` 跳过该插件

### 1.4 Level 不兼容时"全有或全无"

**现状**：[PackageManager.cpp:444-474](file:///d:/projects/language-manager/core/lib/Core/PackageManager.cpp#L444-L474) 中 Level 兼容性检查采用 Strict 模式：

```cpp
for (const auto &info : moduleInfos) {
    auto checkResult = LevelCompatibilityChecker::checkCorePlugin(info.level, levelConfig);
    if (!checkResult.isCompatible) {
        // ...
        return false;  // ← 一个不兼容，全部失败
    }
}
```

**风险**：一个长期未更新的旧插件 Level 不兼容 → 整个系统无法初始化 → 所有插件不可用。

**影响**：直接违反"部分插件长期不更新但要求免维护不能崩溃"的核心需求。

**建议**：

* 改为 Lenient 模式：不兼容的插件跳过并记录警告，其他插件正常加载

* 对核心插件（G2p/Dict）和工具插件（Driver）采用不同策略：

  * 核心插件 Level 不兼容 → 跳过该模块，运行时降级为 copy 模式

  * 工具插件 Level 不兼容 → 跳过该插件，依赖它的模块也一并跳过

* `LevelCompatibilityChecker` 已有 `checkAll()` 方法可批量检查，应利用它收集所有不兼容项而非逐个失败

***

## 二、重要问题（设计不完善）

### 2.1 依赖查找使用硬编码字符串 ID

**现状**：插件通过硬编码字符串查找依赖对象：

```cpp
// LstmG2p 查找 OnnxDriver
auto driverObj = driverCate->getFirstObject("g2pOnnxDriver");

// ChainG2p ModelStep 查找依赖 G2p
auto g2pObj = g2pCate->getFirstObject(m_onnxG2pId);
```

**风险**：

* 依赖 ID 变化 → 静默失败（返回 nullptr → Error）

* 多个同类型对象时 `getFirstObject` 行为不确定

* 依赖关系未在 package.json 中声明，框架无法自动校验

**影响**：插件间耦合依赖隐式字符串约定，core 重构 ID 时旧插件断裂。

**建议**：

* 依赖查找应通过 package.json 的 `dependencies` 声明驱动

* Task 基类提供 `getDependency(category, index)` 方法，基于已解析的依赖声明查找

* 消除 `getFirstObject` 的不确定性，改用依赖声明的精确索引

### 2.2 TaskInput/TaskResult 版本分发缺失

**现状**：`Task::start()` 接受 `NO<TaskInput>` 基类指针，core 不根据 `apiLevel()` 分发不同版本的输入类型。当前所有 Level 的 G2p 插件都使用 `G2pInputV1`，不存在 `G2pInputV2`。

**风险**：

* 如果 core 升级引入 `G2pInputV2`（新字段），旧插件收到 V2 输入后 `as<G2pInputV1>()` 的行为取决于 V2 是否向后兼容 V1 的内存布局

* 没有机制保证 V2 输入能安全降级为 V1 视图

**影响**：Level 升级时输入类型变更可能导致旧插件崩溃。

**建议**：

* 方案 A：规定 V2 必须是 V1 的严格超集（新字段只能追加），保证 `as<V1>()` 安全

* 方案 B：core 在调用 `start()` 前根据 `apiLevel()` 构造对应版本的输入

* 方案 C：输入类型也版本化，`TaskInput` 基类添加 `virtual int version() const`

### 2.3 Manager::convert() 线程不安全

**现状**：`Manager::convert()` 直接访问 `impl.tasks["g2p"]`（`std::map`），无任何锁保护。

**风险**：如果 `initialize()` 和 `convert()` 在不同线程并发调用，存在数据竞争 → UB。

**当前缓解**：设计假设"初始化后只读"的隐式约定。

**影响**：如果未来需要运行时热加载插件，此约定会被打破。

**建议**：

* 短期：在文档/注释中明确线程安全约定

* 长期：`convert()` 中对 tasks 的访问加 `shared_lock`，支持初始化后只读的并发安全

### 2.4 Plugin 单例生命周期与 DLL 绑定

**现状**：`LANGCORE_EXPORT_PLUGIN` 使用函数内 `static` 局部变量：

```cpp
static PLUGIN_NAME _instance;
return &_instance;
```

`_instance` 在 DLL 卸载时销毁，但 core 的 `PluginFactory::allPlugins` 仍持有 `Plugin*` 指针。

**风险**：

* 如果 DLL 被卸载（虽然当前不会），`Plugin*` 变为悬空指针

* DLL 卸载顺序不确定，静态对象析构可能访问已销毁的 core 资源

**影响**：当前"加载后常驻"的设计缓解了此问题，但如果未来需要卸载插件则会崩溃。

**建议**：

* 短期：确认"加载后常驻"为设计约束并文档化

* 长期：`PluginFactory` 在卸载 DLL 前先清除所有引用，或使用弱引用模式

***

## 三、改进建议（设计优化）

### 3.1 VersionedTaskManager 不验证 Level 与实现匹配

**现状**：`TASK_IMPLEMENT` 宏默认使用 V1 实现，不检查 `spec->apiLevel()` 是否有对应实现。LstmG2p 手动 switch 处理了 Level 2，但其他插件如果 package.json 声明 level=2 而插件只有 V1 实现，会静默降级到 V1。

**建议**：`VersionedTaskManager::setImpl()` 时记录实现支持的 Level 范围，`setCurrentLevel()` 时校验是否在范围内，不匹配则返回 Error。

### 3.2 ConfigAccessor 无配置迁移机制

**现状**：插件配置格式变更时，旧配置文件无法自动升级。

**建议**：

* 配置文件添加 `$version` 字段

* `ConfigAccessor` 支持注册迁移函数（`registerMigration(fromVersion, toVersion, migrator)`）

* 加载配置时自动执行迁移链

### 3.3 G2pRes 运行时错误回退静默

**现状**：`Manager::convert()` 降级为 copy 模式时仅写 Critical 日志，上层无法区分"正常 copy"和"错误降级"。

**建议**：`G2pRes` 已有 `G2pErrorType errorType` 字段，降级时应设置对应错误类型（而非默认 `NoError`），上层可据此提示用户。

### 3.4 package.json `class` 字段命名混淆

**现状**：模块声明中的 `class` 字段映射到插件的 `key()`，但 `class` 是 C++/Java 保留字，语义不直观。

**建议**：重命名为 `pluginKey`，向后兼容读取旧字段。

***

## 四、设计优点确认

以下设计决策是合理的，应保持：

| 设计点                          | 评价                  |
| ---------------------------- | ------------------- |
| Level 作为 API 兼容性唯一标准         | 简洁可靠，避免 semver 语义模糊 |
| `Expected<T>` 无异常错误处理        | 性能确定性强，适合插件系统       |
| `VersionedTaskManager` 多版本共存 | 支持 Level 升级期间新旧插件共存 |
| PluginFactory 逐个跳过加载失败       | 良好的错误隔离             |
| SessionSystem 全局缓存 + 引用计数    | 高效的推理资源复用           |
| GPU 降级 CPU 的优雅降级             | 符合"免维护"要求           |
| 配置驱动的 ChainG2p 步骤编排          | 灵活可扩展               |
| `ConfigAccessor` 必需/可选字段区分   | 配置验证清晰              |

***

## 五、优先级排序

| 优先级 | 问题                          | 影响               | 工作量 |
| --- | --------------------------- | ---------------- | --- |
| P0  | 1.4 Level 不兼容全有或全无          | 直接违反核心需求         | 中   |
| P0  | 1.2 `as()` 无类型安全            | Level 升级时旧插件崩溃   | 小   |
| P0  | 1.3 插件加载无崩溃隔离               | 一个坏 DLL 崩溃整个进程   | 小   |
| P1  | 1.1 C++ ABI 跨 DLL 无保护       | 编译器升级导致不兼容       | 大   |
| P1  | 2.1 硬编码依赖 ID                | 重构时隐式断裂          | 中   |
| P1  | 2.2 TaskInput 版本分发缺失        | Level 升级输入类型变更风险 | 中   |
| P2  | 2.3 Manager::convert() 线程安全 | 当前隐式约定缓解         | 小   |
| P2  | 2.4 Plugin 单例生命周期           | 当前"常驻"设计缓解       | 小   |
| P2  | 3.1-3.4 改进建议                | 设计优化             | 小-中 |

***

## 六、实施建议

### 阶段一：紧急修复（P0）

1. **Level 兼容性改为 Lenient 模式**：修改 `checkDependencies()` 使用 `checkAll()` 收集所有不兼容项，跳过不兼容模块而非整体失败
2. **`as()`** **添加类型安全检查**：在 `NamedObject` 添加 `typeInfo()` 虚函数，`as<U>()` 校验类型
3. **插件加载添加 SEH 保护**：Windows 平台 `__try/__except` 包裹 `getter()` 调用

### 阶段二：稳定性加固（P1）

1. **ABI 兼容性校验**：在 `LANGCORE_EXPORT_PLUGIN` 中嵌入编译器/STL 版本标记，加载时校验
2. **依赖查找声明驱动**：通过 package.json dependencies 自动注入依赖对象到 Task
3. **TaskInput 版本安全**：规定 V2 必须是 V1 严格超集，或 core 根据 apiLevel 分发输入

### 阶段三：设计优化（P2）

1. Manager::convert() 线程安全加固
2. VersionedTaskManager Level 校验
3. ConfigAccessor 配置迁移
4. G2pRes 错误回退标记

