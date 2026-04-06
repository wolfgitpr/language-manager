// ============================================================================
// Task 引用计数系统使用示例
// ============================================================================

#include <LangCore/Task/Task.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Core/Manager.h>
#include <LangCore/Support/Logging.h>

using namespace LangCore;

namespace Examples
{

// ============================================================================
// 示例 1：检查 Task 是否被多个 Task 引用
// ============================================================================
void example1_CheckIfTaskIsShared()
{
    // 获取 Manager 实例
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    if (!taskExp) {
        LOG_ERROR("Failed to get task: {}", taskExp.error().message());
        return;
    }
    
    auto task = taskExp.value();
    
    // 检查是否被多个 Task 引用
    if (task->isShared()) {
        LOG_WARNING("Task splitter-eng is shared by {} tasks", 
                   task->spec()->refCount());
        
        // 获取引用者列表
        auto referrers = task->spec()->referrers();
        for (const auto &referrer : referrers) {
            LOG_INFO("  - Referenced by: {}::{} ({})", 
                    referrer.packageId, 
                    referrer.moduleId, 
                    referrer.name);
        }
    } else {
        LOG_INFO("Task splitter-eng is not shared");
    }
}

// ============================================================================
// 示例 2：获取 Task 的引用信息并显示警告
// ============================================================================
void example2_ShowReferenceWarning()
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    if (!taskExp) {
        return;
    }
    
    auto task = taskExp.value();
    
    // 获取引用信息
    auto refInfo = task->spec()->getReferenceInfo();
    
    // 检查是否被多个 Task 引用
    bool isShared = refInfo["isShared"].toBool(false);
    if (isShared) {
        int refCount = refInfo["refCount"].toInt(0);
        
        LOG_WARNING("⚠️  This task is used by {} tasks", refCount);
        LOG_WARNING("Modifying settings may affect the following tasks:");
        
        // 显示引用者列表
        auto referrers = refInfo["referrers"].toArray();
        for (const auto &referrer : referrers) {
            auto referrerObj = referrer.toObject();
            std::string name = referrerObj["name"].toString();
            std::string packageId = referrerObj["packageId"].toString();
            std::string moduleId = referrerObj["moduleId"].toString();
            
            LOG_WARNING("  • {} ({}::{})", name, packageId, moduleId);
        }
    }
}

// ============================================================================
// 示例 3：生成包含引用信息的 UI Schema
// ============================================================================
std::string example3_GenerateUiSchemaWithReferenceInfo()
{
    auto mgr = Manager::instance();
    
    // 获取 Task
    auto taskExp = mgr->task("splitter", "splitter-eng");
    if (!taskExp) {
        return "{}";
    }
    
    auto task = taskExp.value();
    
    // 获取 UI Schema（自动包含引用信息）
    std::string uiSchema = task->getUiSchema();
    
    // 解析并显示引用信息
    auto schema = JsonValue::fromJson(uiSchema);
    auto schemaObj = schema.toObject();
    
    if (schemaObj.contains("referenceInfo")) {
        auto refInfo = schemaObj["referenceInfo"].toObject();
        
        LOG_INFO("UI Schema includes reference info:");
        LOG_INFO("  - Reference Count: {}", refInfo["refCount"].toInt(0));
        LOG_INFO("  - Is Shared: {}", refInfo["isShared"].toBool(false));
        
        if (refInfo.contains("warning")) {
            LOG_INFO("  - Warning: Yes");
        }
    }
    
    return uiSchema;
}

// ============================================================================
// 示例 4：在 Task 配置界面中显示引用信息
// ============================================================================
class TaskConfigWidget
{
public:
    void loadTask(const std::string &category, const std::string &id)
    {
        auto mgr = Manager::instance();
        auto taskExp = mgr->task(category, id);
        
        if (!taskExp) {
            LOG_ERROR("Failed to load task: {}", taskExp.error().message());
            return;
        }
        
        m_task = taskExp.value();
        
        // 加载 UI Schema
        std::string uiSchema = m_task->getUiSchema();
        auto schema = JsonValue::fromJson(uiSchema);
        
        // 渲染引用信息
        renderReferenceInfo(schema.toObject());
        
        // 渲染配置表单
        renderConfigForm(schema.toObject());
    }
    
private:
    void renderReferenceInfo(const JsonObject &schema)
    {
        if (!schema.contains("referenceInfo")) {
            return;
        }
        
        auto refInfo = schema["referenceInfo"].toObject();
        int refCount = refInfo["refCount"].toInt(0);
        bool isShared = refInfo["isShared"].toBool(false);
        
        // 显示引用计数
        std::cout << "Reference Count: " << refCount << std::endl;
        
        // 如果被多个 Task 引用，显示警告
        if (isShared) {
            renderSharedTaskWarning(refInfo);
        }
    }
    
    void renderSharedTaskWarning(const JsonObject &refInfo)
    {
        std::cout << "⚠️  Warning: This task is used by multiple tasks" << std::endl;
        std::cout << "Modifying settings may affect the following tasks:" << std::endl;
        
        auto referrers = refInfo["referrers"].toArray();
        for (const auto &referrer : referrers) {
            auto referrerObj = referrer.toObject();
            std::string name = referrerObj["name"].toString();
            std::string packageId = referrerObj["packageId"].toString();
            std::string moduleId = referrerObj["moduleId"].toString();
            
            std::cout << "  • " << name << " (" << packageId << "::" << moduleId << ")" << std::endl;
        }
    }
    
    void renderConfigForm(const JsonObject &schema)
    {
        // 渲染配置表单...
        std::cout << "Configuration Form:" << std::endl;
    }
    
    NO<Task> m_task;
};

// ============================================================================
// 示例 5：处理包卸载时的引用计数更新
// ============================================================================
void example5_HandlePackageUnload(const std::string &packageId)
{
    auto mgr = Manager::instance();
    
    // 获取包中的所有模块
    auto modules = mgr->getPackageModules(packageId);
    
    LOG_INFO("Unloading package: {}", packageId);
    LOG_INFO("  Modules: {}", modules.size());
    
    // 显示将被更新引用计数的模块
    for (const auto &module : modules) {
        LOG_INFO("  - {}::{}", module.packageId, module.moduleId);
        
        // 显示该模块的依赖
        for (const auto &dep : module.resolvedDependencies) {
            auto depTaskExp = mgr->task(dep.moduleId, dep.moduleId);
            if (depTaskExp) {
                auto depTask = depTaskExp.value();
                int oldRefCount = depTask->spec()->refCount();
                int newRefCount = oldRefCount - 1;
                
                LOG_INFO("    Updating reference count for {}::{}: {} -> {}", 
                        dep.packageId, dep.moduleId, 
                        oldRefCount, newRefCount);
            }
        }
    }
    
    // 卸载包
    // mgr->unloadPackage(packageId);
}

// ============================================================================
// 示例 6：遍历所有 Task 并显示引用统计
// ============================================================================
void example6_ShowAllTaskReferences()
{
    auto mgr = Manager::instance();
    
    // 获取所有类别
    auto categories = mgr->categories();
    
    std::cout << "Task Reference Statistics:" << std::endl;
    std::cout << "================================" << std::endl;
    
    int totalTasks = 0;
    int sharedTasks = 0;
    
    for (const auto &category : categories) {
        std::cout << "\nCategory: " << category->name() << std::endl;
        
        // 获取该类别中的所有模块
        auto specs = category->specs();
        
        for (const auto &spec : specs) {
            totalTasks++;
            
            int refCount = spec->refCount();
            bool isShared = spec->isShared();
            
            if (isShared) {
                sharedTasks++;
                std::cout << "  ⚠️  " << spec->id() 
                         << " (refCount: " << refCount << ")" << std::endl;
                
                // 显示引用者
                auto referrers = spec->referrers();
                for (const auto &referrer : referrers) {
                    std::cout << "      ← " << referrer.name 
                             << " (" << referrer.packageId << "::" 
                             << referrer.moduleId << ")" << std::endl;
                }
            } else {
                std::cout << "  ✓  " << spec->id() 
                         << " (refCount: " << refCount << ")" << std::endl;
            }
        }
    }
    
    std::cout << "\n================================" << std::endl;
    std::cout << "Total Tasks: " << totalTasks << std::endl;
    std::cout << "Shared Tasks: " << sharedTasks << std::endl;
    std::cout << "Shared Task Ratio: " 
              << (totalTasks > 0 ? (sharedTasks * 100.0 / totalTasks) : 0.0) 
              << "%" << std::endl;
}

// ============================================================================
// 示例 7：在热重载时重新计算引用计数
// ============================================================================
void example7_HandleHotReload(const std::string &packageId)
{
    auto mgr = Manager::instance();
    
    LOG_INFO("Hot reloading package: {}", packageId);
    
    // 1. 卸载旧包
    LOG_INFO("Step 1: Unloading old package...");
    // mgr->unloadPackage(packageId);
    
    // 2. 加载新包
    LOG_INFO("Step 2: Loading new package...");
    // auto result = mgr->loadPackage(packageId);
    
    // 3. 重新计算引用计数
    LOG_INFO("Step 3: Recalculating reference counts...");
    auto graph = mgr->dependencyGraph();
    graph->updateAllRefCounts();
    
    // 4. 验证引用计数
    LOG_INFO("Step 4: Verifying reference counts...");
    example6_ShowAllTaskReferences();
    
    // 5. 通知 UI 更新
    LOG_INFO("Step 5: Notifying UI update...");
    // mgr->notifyUiUpdate();
}

// ============================================================================
// 示例 8：自定义 Task 实现中的引用信息处理
// ============================================================================
class CustomTask : public Task
{
public:
    explicit CustomTask(const ModuleSpec *spec) : Task(spec) {}
    
    int apiLevel() const override { return 1; }
    
    Expected<void> initialize() override
    {
        // 初始化逻辑...
        
        // 检查是否被多个 Task 引用
        if (isShared()) {
            LOG_WARNING("⚠️  This task is shared by {} tasks", 
                       spec()->refCount());
            
            // 记录引用者
            auto referrers = spec()->referrers();
            for (const auto &referrer : referrers) {
                LOG_INFO("  Referenced by: {}", referrer.name);
            }
        }
        
        return {};
    }
    
    Expected<NO<TaskResult>> start(const NO<TaskInput> &input) override
    {
        // 执行逻辑...
        return {};
    }
    
    // 覆盖 getUiSchema 以添加自定义引用信息
    std::string getUiSchema() const override
    {
        // 获取基础 UI Schema
        auto baseSchema = Task::getUiSchema();
        
        // 添加自定义信息
        auto schemaJson = JsonValue::fromJson(baseSchema);
        auto schemaObj = schemaJson.toObject();
        
        // 添加性能警告（如果被多个 Task 引用）
        if (isShared()) {
            auto perfWarning = JsonObject();
            perfWarning["type"] = "performance";
            perfWarning["message"] = "This task is shared. Consider caching results.";
            perfWarning["severity"] = "info";
            
            schemaObj["performanceInfo"] = perfWarning;
        }
        
        return schemaObj.toJson();
    }
};

// ============================================================================
// 示例 9：测试引用计数计算的正确性
// ============================================================================
void example9_VerifyReferenceCounts()
{
    auto mgr = Manager::instance();
    
    LOG_INFO("Verifying reference count calculations...");
    
    // 获取所有模块
    auto graph = mgr->dependencyGraph();
    auto allModules = graph->getAllModules();
    
    // 遍历所有模块
    for (const auto &module : allModules) {
        // 计算期望的引用计数
        int expectedRefCount = graph->calculateRefCount(
            module.packageId, 
            module.moduleId
        );
        
        // 获取实际的引用计数
        auto spec = mgr->findModuleSpec(module.packageId, module.moduleId);
        if (spec) {
            int actualRefCount = spec->refCount();
            
            // 比较
            if (expectedRefCount != actualRefCount) {
                LOG_ERROR("❌  Reference count mismatch for {}::{}: expected={}, actual={}", 
                         module.packageId, module.moduleId, 
                         expectedRefCount, actualRefCount);
            } else {
                LOG_INFO("✓  {}::{}: refCount={}", 
                        module.packageId, module.moduleId, 
                        actualRefCount);
            }
        }
    }
}

// ============================================================================
// 示例 10：显示引用关系图
// ============================================================================
void example10_ShowReferenceGraph()
{
    auto mgr = Manager::instance();
    auto graph = mgr->dependencyGraph();
    auto allModules = graph->getAllModules();
    
    std::cout << "Reference Graph:" << std::endl;
    std::cout << "=================" << std::endl;
    
    for (const auto &module : allModules) {
        std::string moduleKey = module.packageId + "::" + module.moduleId;
        
        // 显示模块
        std::cout << moduleKey << " (refCount: " << module.refCount << ")" << std::endl;
        
        // 显示依赖
        if (!module.resolvedDependencies.empty()) {
            std::cout << "  depends on:" << std::endl;
            for (const auto &dep : module.resolvedDependencies) {
                std::cout << "    → " << dep.packageId << "::" << dep.moduleId << std::endl;
            }
        }
        
        // 显示引用者
        auto referrers = graph->getReferrers(module.packageId, module.moduleId);
        if (!referrers.empty()) {
            std::cout << "  referenced by:" << std::endl;
            for (const auto &referrer : referrers) {
                std::cout << "    ← " << referrer.packageId << "::" << referrer.moduleId << std::endl;
            }
        }
        
        std::cout << std::endl;
    }
}

// ============================================================================
// 主函数：运行所有示例
// ============================================================================
int main()
{
    // 初始化 Manager
    auto mgr = Manager::instance();
    std::string errMsg;
    if (!mgr->initialize(errMsg)) {
        LOG_ERROR("Failed to initialize Manager: {}", errMsg);
        return -1;
    }
    
    // 运行示例
    example1_CheckIfTaskIsShared();
    std::cout << std::endl;
    
    example2_ShowReferenceWarning();
    std::cout << std::endl;
    
    std::string uiSchema = example3_GenerateUiSchemaWithReferenceInfo();
    std::cout << std::endl;
    
    example6_ShowAllTaskReferences();
    std::cout << std::endl;
    
    example9_VerifyReferenceCounts();
    std::cout << std::endl;
    
    example10_ShowReferenceGraph();
    
    return 0;
}

} // namespace Examples