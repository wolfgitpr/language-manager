#ifndef LANGCORE_VERSIONEDTASKMANAGER_H
#define LANGCORE_VERSIONEDTASKMANAGER_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangCore
{
    /// VersionedTaskManager - 多版本任务管理辅助类
    ///
    /// 这个类为多版本插件提供基本的版本管理功能。
    /// 它封装了版本存储和实现委托的通用模式。
    ///
    /// 设计原则：
    /// - 只提供数据结构和辅助方法
    /// - 不包含具体的版本选择逻辑
    /// - 允许插件自定义版本选择策略
    template<typename TaskType>
    class VersionedTaskManager {
    public:
        /// VersionedImpl - 版本化实现的存储结构
        struct VersionedImpl {
            const ModuleSpec *spec = nullptr;          ///< 模块规范
            std::unique_ptr<VersionedTaskImplBase> impl; ///< 任务实现
            int currentLevel = 1;                      ///< 当前 API Level

            explicit VersionedImpl(const ModuleSpec *s) : spec(s) {}
        };

        /// 构造函数
        /// @param spec 模块规范
        explicit VersionedTaskManager(const ModuleSpec *spec)
            : _impl(std::make_unique<VersionedImpl>(spec)) {}

        /// 获取当前 API Level
        /// @return 当前 API Level
        int currentLevel() const {
            return _impl->currentLevel;
        }

        /// 设置当前 API Level
        /// @param level API Level
        void setCurrentLevel(int level) {
            _impl->currentLevel = level;
        }

        /// 获取模块规范
        /// @return 模块规范指针
        const ModuleSpec *spec() const {
            return _impl->spec;
        }

        /// 获取实现
        /// @return 实现指针
        VersionedTaskImplBase *impl() const {
            return _impl->impl.get();
        }

        /// 设置实现
        /// @param impl 实现对象
        void setImpl(std::unique_ptr<VersionedTaskImplBase> impl) {
            _impl->impl = std::move(impl);
        }

        /// 初始化任务
        /// @return 成功返回 Expected<void>::success()，失败返回错误信息
        Expected<void> initialize() {
            return _impl->impl->initialize();
        }

        /// 执行任务
        /// @param input 任务输入数据
        /// @return 成功返回任务结果，失败返回错误信息
        Expected<NO<TaskResult>>
        start(const NO<TaskInput> &input) {
            return _impl->impl->start(input);
        }

        /// 获取配置
        /// @return JSON 格式的配置字符串
        std::string getConfig() const {
            return _impl->impl->getConfig();
        }

    protected:
        std::unique_ptr<VersionedImpl> _impl;
    };

    // Macro to simplify plugin task implementation
    // Usage: TASK_IMPLEMENT(TaskClass, ManagerClass, ImplNamespace, ImplClass)
    //
    // This macro implements the standard task methods that delegate to a VersionedTaskManager.
    // It reduces boilerplate code in plugin task implementations.
    //
    // Example:
    //   namespace MyPlugin {
    //       class MyTask : public LangCore::Task {
    //       public:
    //           TASK_IMPLEMENT(MyTask, VersionedTaskManager<MyTask>, Internal::V1, MyTaskImpl)
    //       };
    //   }
    #define TASK_IMPLEMENT(TaskClass, ManagerClass, ImplNamespace, ImplClass) \
        TaskClass::TaskClass(const LangCore::ModuleSpec *spec) \
            : LangCore::Task(spec), _manager(spec) { \
            int level = spec->apiLevel(); \
            _manager.setCurrentLevel(level); \
            _manager.setImpl(std::make_unique<ImplNamespace::ImplClass>(spec)); \
        } \
        \
        TaskClass::~TaskClass() = default; \
        \
        int TaskClass::apiLevel() const { \
            return _manager.currentLevel(); \
        } \
        \
        LangCore::Expected<void> TaskClass::initialize() { \
            return _manager.initialize(); \
        } \
        \
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>> \
        TaskClass::start(const LangCore::NO<LangCore::TaskInput> &input) { \
            return _manager.start(input); \
        } \
        \
        std::string TaskClass::getConfig() const { \
            return _manager.getConfig(); \
        }

} // namespace LangCore

#endif // LANGCORE_VERSIONEDTASKMANAGER_H