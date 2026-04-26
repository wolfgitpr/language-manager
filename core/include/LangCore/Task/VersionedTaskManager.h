#ifndef LANGCORE_VERSIONEDTASKMANAGER_H
#define LANGCORE_VERSIONEDTASKMANAGER_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangCore
{
    /// VersionedTaskManager - 多版本任务管理辅助类
    ///
    /// 持有一个 VersionedTaskImplBase 实现并委托 initialize/start/getConfig。
    /// 插件在构造函数中根据 spec->apiLevel() 选择实现并调用 setImpl()。
    class VersionedTaskManager {
    public:
        explicit VersionedTaskManager(const ModuleSpec *spec)
            : _spec(spec), _currentLevel(spec ? spec->apiLevel() : 1) {}

        int currentLevel() const { return _currentLevel; }
        const ModuleSpec *spec() const { return _spec; }
        VersionedTaskImplBase *impl() const { return _impl.get(); }

        void setImpl(std::unique_ptr<VersionedTaskImplBase> impl) {
            _impl = std::move(impl);
        }

        Expected<void> initialize() {
            if (!_impl)
                return Error(Error::NullPointerError, "VersionedTaskManager: impl not set (call setImpl() first)");
            return _impl->initialize();
        }

        Expected<NO<TaskResult>> start(const NO<TaskInput> &input) {
            if (!_impl)
                return Error(Error::NullPointerError, "VersionedTaskManager: impl not set (call setImpl() first)");
            return _impl->start(input);
        }

        std::string getConfig() const {
            if (!_impl)
                return {};
            return _impl->getConfig();
        }

    private:
        const ModuleSpec *_spec;
        int _currentLevel;
        std::unique_ptr<VersionedTaskImplBase> _impl;
    };

    /// TASK_IMPLEMENT - 为使用 VersionedTaskManager 的 Task 生成标准委托方法。
    ///
    /// 用法（单版本）:
    ///   TASK_IMPLEMENT(MyTask, Internal::V1::MyTaskImpl)
    ///
    /// 生成: 构造函数、析构函数、apiLevel、initialize、start、getConfig
    /// 构造函数中自动创建指定的 Impl 类。
    ///
    /// 对于多版本插件，请手动编写构造函数（使用 switch 选择 Impl），
    /// 并使用 TASK_IMPLEMENT_METHODS 仅生成委托方法。
    #define TASK_IMPLEMENT(TaskClass, ImplClass) \
        TaskClass::TaskClass(const LangCore::ModuleSpec *spec) \
            : LangCore::Task(spec), _manager(spec) { \
            _manager.setImpl(std::make_unique<ImplClass>(spec)); \
        } \
        TASK_IMPLEMENT_METHODS(TaskClass)

    /// TASK_IMPLEMENT_METHODS - 仅生成委托方法（不含构造函数）。
    /// 供多版本插件使用：手动编写构造函数，然后调用此宏。
    #define TASK_IMPLEMENT_METHODS(TaskClass) \
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
