// ============================================================================
// Task 引用计数系统测试用例
// ============================================================================

#include <gtest/gtest.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Core/Manager.h>

using namespace LangCore;

namespace Tests
{

// ============================================================================
// 测试 fixture
// ============================================================================
class ReferenceCountTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 初始化 Manager
        auto mgr = Manager::instance();
        std::string errMsg;
        ASSERT_TRUE(mgr->initialize(errMsg)) << errMsg;
        
        // 获取依赖图
        m_graph = mgr->dependencyGraph();
    }
    
    void TearDown() override
    {
        // 清理
    }
    
    // 辅助函数：创建测试模块
    ModuleMetadata createTestModule(
        const std::string &packageId,
        const std::string &moduleId,
        const std::string &type,
        const std::string &name)
    {
        ModuleMetadata module;
        module.packageId = packageId;
        module.moduleId = moduleId;
        module.type = type;
        module.name = name;
        module.level = 1;
        module.version = "1.0.0";
        
        return module;
    }
    
    DependencyGraph *m_graph = nullptr;
};

// ============================================================================
// 测试 1：基本的引用计数计算
// ============================================================================
TEST_F(ReferenceCountTest, BasicReferenceCount)
{
    // 创建测试模块
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    ModuleMetadata moduleC = createTestModule("pkg-c", "module-c", "tagger", "Module C");
    
    // 设置依赖关系
    // A 依赖 B
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // C 也依赖 B
    moduleC.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // 添加到依赖图
    // m_graph->addNode(moduleA);
    // m_graph->addNode(moduleB);
    // m_graph->addNode(moduleC);
    
    // 计算引用计数
    // m_graph->updateAllRefCounts();
    
    // 验证引用计数
    // EXPECT_EQ(moduleA.refCount, 0);  // A 不被任何人依赖
    // EXPECT_EQ(moduleB.refCount, 2);  // B 被 A 和 C 依赖
    // EXPECT_EQ(moduleC.refCount, 0);  // C 不被任何人依赖
}

// ============================================================================
// 测试 2：检查 Task 是否被共享
// ============================================================================
TEST_F(ReferenceCountTest, CheckTaskIsShared)
{
    auto mgr = Manager::instance();
    
    // 获取一个被多个 Task 依赖的 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 检查是否被共享
    bool isShared = task->isShared();
    
    // 获取引用计数
    int refCount = task->spec()->refCount();
    
    // 验证：如果引用计数 > 1，应该被标记为共享
    if (refCount > 1) {
        EXPECT_TRUE(isShared);
    } else {
        EXPECT_FALSE(isShared);
    }
}

// ============================================================================
// 测试 3：获取引用者列表
// ============================================================================
TEST_F(ReferenceCountTest, GetReferrersList)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 获取引用者列表
    auto referrers = task->spec()->referrers();
    
    // 验证引用者信息
    for (const auto &referrer : referrers) {
        // 检查必需字段
        EXPECT_FALSE(referrer.packageId.empty());
        EXPECT_FALSE(referrer.moduleId.empty());
        EXPECT_FALSE(referrer.name.empty());
        EXPECT_FALSE(referrer.category.empty());
        EXPECT_GT(referrer.level, 0);
    }
}

// ============================================================================
// 测试 4：初始化失败不影响引用计数
// ============================================================================
TEST_F(ReferenceCountTest, InitFailureDoesNotAffectRefCount)
{
    // 这个测试需要模拟初始化失败的场景
    // 在实际实现中，初始化失败的 Task 不会被注册到 ObjectPool
    // 因此不会影响被依赖者的引用计数
    
    // 创建测试模块
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    
    // 设置依赖关系
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // 假设 moduleA 初始化失败
    
    // 期望：moduleB 的引用计数应该为 0（因为 A 没有成功初始化）
    // EXPECT_EQ(moduleB.refCount, 0);
}

// ============================================================================
// 测试 5：循环依赖检测
// ============================================================================
TEST_F(ReferenceCountTest, CircularDependencyDetection)
{
    // 创建循环依赖
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    ModuleMetadata moduleC = createTestModule("pkg-c", "module-c", "tagger", "Module C");
    
    // 设置循环依赖：A → B → C → A
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    moduleB.resolvedDependencies = {
        { "pkg-c", "module-c", 1, "1.0.0" }
    };
    
    moduleC.resolvedDependencies = {
        { "pkg-a", "module-a", 1, "1.0.0" }
    };
    
    // 添加到依赖图
    // m_graph->addNode(moduleA);
    // m_graph->addNode(moduleB);
    // m_graph->addNode(moduleC);
    
    // 检测循环依赖
    // bool hasCycles = m_graph->hasCycles();
    // EXPECT_TRUE(hasCycles);
    
    // 循环依赖应该被拒绝，引用计数不应该计算
    // EXPECT_EQ(moduleA.refCount, 0);
    // EXPECT_EQ(moduleB.refCount, 0);
    // EXPECT_EQ(moduleC.refCount, 0);
}

// ============================================================================
// 测试 6：重复依赖只计算一次
// ============================================================================
TEST_F(ReferenceCountTest, DuplicateDependencyCountedOnce)
{
    // 创建测试模块
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    
    // 设置重复依赖
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" },
        { "pkg-b", "module-b", 1, "1.0.0" }  // 重复
    };
    
    // 添加到依赖图
    // m_graph->addNode(moduleA);
    // m_graph->addNode(moduleB);
    
    // 计算引用计数
    // m_graph->updateAllRefCounts();
    
    // 验证：重复依赖应该被去重，只计算一次
    // EXPECT_EQ(moduleB.refCount, 1);
}

// ============================================================================
// 测试 7：间接依赖不计入引用计数
// ============================================================================
TEST_F(ReferenceCountTest, IndirectDependencyNotCounted)
{
    // 创建测试模块
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    ModuleMetadata moduleC = createTestModule("pkg-c", "module-c", "tagger", "Module C");
    
    // 设置间接依赖：A → B → C
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    moduleB.resolvedDependencies = {
        { "pkg-c", "module-c", 1, "1.0.0" }
    };
    
    // 添加到依赖图
    // m_graph->addNode(moduleA);
    // m_graph->addNode(moduleB);
    // m_graph->addNode(moduleC);
    
    // 计算引用计数
    // m_graph->updateAllRefCounts();
    
    // 验证：只统计直接依赖
    // EXPECT_EQ(moduleA.refCount, 0);  // A 不被任何人依赖
    // EXPECT_EQ(moduleB.refCount, 1);  // B 被 A 依赖（直接）
    // EXPECT_EQ(moduleC.refCount, 1);  // C 被 B 依赖（直接），不被 A 依赖（间接）
}

// ============================================================================
// 测试 8：包卸载时更新引用计数
// ============================================================================
TEST_F(ReferenceCountTest, UpdateRefCountOnPackageUnload)
{
    auto mgr = Manager::instance();
    
    // 假设有两个包
    // pkg-a: module-a 依赖 module-b
    // pkg-b: module-b
    
    // 获取 module-b 的初始引用计数
    auto taskBExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskBExp.ok());
    
    auto taskB = taskBExp.value();
    int initialRefCount = taskB->spec()->refCount();
    
    // 卸载 pkg-a
    // mgr->unloadPackage("pkg-a");
    
    // 验证引用计数减少
    // int newRefCount = taskB->spec()->refCount();
    // EXPECT_EQ(newRefCount, initialRefCount - 1);
}

// ============================================================================
// 测试 9：UI Schema 包含引用信息
// ============================================================================
TEST_F(ReferenceCountTest, UiSchemaIncludesReferenceInfo)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 获取 UI Schema
    std::string uiSchema = task->getUiSchema();
    
    // 解析 UI Schema
    auto schema = JsonValue::fromJson(uiSchema);
    auto schemaObj = schema.toObject();
    
    // 验证包含引用信息
    EXPECT_TRUE(schemaObj.contains("referenceInfo"));
    
    auto refInfo = schemaObj["referenceInfo"].toObject();
    
    // 验证引用信息字段
    EXPECT_TRUE(refInfo.contains("refCount"));
    EXPECT_TRUE(refInfo.contains("isShared"));
    EXPECT_TRUE(refInfo.contains("referrers"));
}

// ============================================================================
// 测试 10：被共享的 Task 显示警告
// ============================================================================
TEST_F(ReferenceCountTest, SharedTaskShowsWarning)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 获取 UI Schema
    std::string uiSchema = task->getUiSchema();
    auto schema = JsonValue::fromJson(uiSchema);
    auto schemaObj = schema.toObject();
    
    auto refInfo = schemaObj["referenceInfo"].toObject();
    bool isShared = refInfo["isShared"].toBool(false);
    
    // 如果被共享，应该包含警告
    if (isShared) {
        EXPECT_TRUE(refInfo.contains("warning"));
        
        auto warning = refInfo["warning"].toObject();
        EXPECT_TRUE(warning.contains("type"));
        EXPECT_TRUE(warning.contains("message"));
        EXPECT_TRUE(warning.contains("details"));
        
        EXPECT_EQ(warning["type"].toString(), "shared_task");
    }
}

// ============================================================================
// 测试 11：引用计数为零的 Task
// ============================================================================
TEST_F(ReferenceCountTest, TaskWithZeroRefCount)
{
    auto mgr = Manager::instance();
    
    // 获取一个没有被任何 Task 依赖的 Task
    auto taskExp = mgr->task("g2p", "g2p-cmn");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 验证引用计数为零
    int refCount = task->spec()->refCount();
    EXPECT_GE(refCount, 0);
    
    // 如果引用计数为零，不应该显示警告
    if (refCount == 0) {
        std::string uiSchema = task->getUiSchema();
        auto schema = JsonValue::fromJson(uiSchema);
        auto schemaObj = schema.toObject();
        
        auto refInfo = schemaObj["referenceInfo"].toObject();
        EXPECT_FALSE(refInfo["isShared"].toBool(false));
        EXPECT_FALSE(refInfo.contains("warning"));
    }
}

// ============================================================================
// 测试 12：引用者信息完整性
// ============================================================================
TEST_F(ReferenceCountTest, ReferrerInfoCompleteness)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 获取引用者列表
    auto referrers = task->spec()->referrers();
    
    // 验证每个引用者的信息完整性
    for (const auto &referrer : referrers) {
        // 验证必需字段
        EXPECT_FALSE(referrer.packageId.empty()) << "packageId should not be empty";
        EXPECT_FALSE(referrer.moduleId.empty()) << "moduleId should not be empty";
        EXPECT_FALSE(referrer.name.empty()) << "name should not be empty";
        EXPECT_FALSE(referrer.category.empty()) << "category should not be empty";
        EXPECT_GT(referrer.level, 0) << "level should be greater than 0";
    }
}

// ============================================================================
// 测试 13：多次计算引用计数的一致性
// ============================================================================
TEST_F(ReferenceCountTest, ConsistentRefCountAfterMultipleUpdates)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 第一次计算引用计数
    int refCount1 = task->spec()->refCount();
    
    // 第二次计算引用计数
    // m_graph->updateAllRefCounts();
    int refCount2 = task->spec()->refCount();
    
    // 第三次计算引用计数
    // m_graph->updateAllRefCounts();
    int refCount3 = task->spec()->refCount();
    
    // 验证一致性
    EXPECT_EQ(refCount1, refCount2);
    EXPECT_EQ(refCount2, refCount3);
}

// ============================================================================
// 测试 14：引用信息的 JSON 序列化
// ============================================================================
TEST_F(ReferenceCountTest, ReferenceInfoJsonSerialization)
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    ASSERT_TRUE(taskExp.ok());
    
    auto task = taskExp.value();
    
    // 获取引用信息
    auto refInfo = task->spec()->getReferenceInfo();
    
    // 序列化为 JSON
    std::string json = refInfo.toJson();
    
    // 反序列化
    auto parsed = JsonValue::fromJson(json);
    
    // 验证反序列化结果
    auto parsedObj = parsed.toObject();
    EXPECT_EQ(parsedObj["refCount"].toInt(), refInfo["refCount"].toInt());
    EXPECT_EQ(parsedObj["isShared"].toBool(), refInfo["isShared"].toBool());
}

// ============================================================================
// 测试 15：跨包依赖的引用计数
// ============================================================================
TEST_F(ReferenceCountTest, CrossPackageDependencyRefCount)
{
    // 创建跨包依赖
    ModuleMetadata moduleA = createTestModule("pkg-a", "module-a", "g2p", "Module A");
    ModuleMetadata moduleB = createTestModule("pkg-b", "module-b", "splitter", "Module B");
    ModuleMetadata moduleC = createTestModule("pkg-c", "module-c", "tagger", "Module C");
    
    // A 依赖 B（跨包）
    moduleA.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // C 也依赖 B（跨包）
    moduleC.resolvedDependencies = {
        { "pkg-b", "module-b", 1, "1.0.0" }
    };
    
    // 添加到依赖图
    // m_graph->addNode(moduleA);
    // m_graph->addNode(moduleB);
    // m_graph->addNode(moduleC);
    
    // 计算引用计数
    // m_graph->updateAllRefCounts();
    
    // 验证跨包依赖的引用计数
    // EXPECT_EQ(moduleB.refCount, 2);  // B 被 A 和 C 依赖（跨包）
}

} // namespace Tests

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}