#ifndef LANGCORE_VERSIONEDTASKSELECTOR_H
#define LANGCORE_VERSIONEDTASKSELECTOR_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangCore
{
    /// LANGPLUGINS_SELECT_VERSION - 版本选择宏
    ///
    /// 这个宏简化了插件中的版本选择逻辑，提供统一的版本选择模式。
    ///
    /// 使用示例：
    /// \code
    /// CantoneseG2pTask::CantoneseG2pTask(const LangCore::ModuleSpec *spec)
    ///     : LangCore::Task(spec), _manager(spec) {
    ///
    ///     LANGPLUGINS_SELECT_VERSION(spec,
    ///         LANGPLUGINS_VERSION_ENTRY(1, Internal::V1::CantoneseG2pTaskImpl),
    ///         LANGPLUGINS_VERSION_ENTRY(2, Internal::V2::CantoneseG2pTaskImpl)
    ///     );
    /// }
    /// \endcode
    ///
    /// @param spec 模块规范
    /// @param ... 版本条目（使用 LANGPLUGINS_VERSION_ENTRY 宏）
    #define LANGPLUGINS_SELECT_VERSION(spec, ...) \
        do { \
            int level = (spec)->apiLevel(); \
            _manager.setCurrentLevel(level); \
            \
            auto impl = LangCore::selectVersionImpl( \
                spec, \
                level, \
                __VA_ARGS__ \
            ); \
            \
            if (impl) { \
                _manager.setImpl(std::move(impl)); \
            } else { \
                /* 默认使用 Level 1 */ \
                _manager.setImpl(std::make_unique<LangCore::DefaultVersionImpl>(spec)); \
            } \
        } while (0)

    /// LANGPLUGINS_VERSION_ENTRY - 版本条目宏
    ///
    /// 定义一个版本条目，用于 LANGPLUGINS_SELECT_VERSION 宏。
    ///
    /// @param level 版本号
    /// @param ImplType 实现类类型
    #define LANGPLUGINS_VERSION_ENTRY(level, ImplType) \
        std::make_pair(level, [](const LangCore::ModuleSpec *spec) -> std::unique_ptr<LangCore::VersionedTaskImplBase> { \
            return std::make_unique<ImplType>(spec); \
        })

    /// selectVersionImpl - 版本选择函数
    ///
    /// 根据指定的 Level 选择对应的实现类。
    ///
    /// @param spec 模块规范
    /// @param level 目标 Level
    /// @param ... 版本条目（使用 LANGPLUGINS_VERSION_ENTRY 宏）
    /// @return 实现对象，如果未找到则返回 nullptr
    template<typename... VersionEntries>
    std::unique_ptr<VersionedTaskImplBase> selectVersionImpl(
        const LangCore::ModuleSpec *spec,
        int level,
        VersionEntries... entries
    ) {
        // 将版本条目转换为数组
        std::pair<int, std::function<std::unique_ptr<VersionedTaskImplBase>(const LangCore::ModuleSpec *)>> versionMap[] = {
            entries...
        };

        // 查找匹配的版本
        for (const auto &entry : versionMap) {
            if (entry.first == level) {
                return entry.second(spec); // 传递正确的 spec 参数
            }
        }

        return nullptr;
    }

    /// DefaultVersionImpl - 默认版本实现
    ///
    /// 当找不到匹配的版本时使用的默认实现（Level 1）。
    ///
    class DefaultVersionImpl : public VersionedTaskImplBase {
    public:
        explicit DefaultVersionImpl(const LangCore::ModuleSpec *spec) : m_spec(spec) {}

        Expected<void> initialize() override {
            // 默认实现：返回错误
            return Error(Error::NotImplementedError,
                        "No implementation available for this level");
        }

        Expected<NO<TaskResult>> start(const NO<TaskInput> &input) override {
            // 默认实现：返回错误
            return Error(Error::NotImplementedError,
                        "No implementation available for this level");
        }

        std::string getConfig() const override {
            return "{}";
        }

        Expected<void> setConfig(const std::string &config) override {
            return {};
        }

    private:
        const LangCore::ModuleSpec *m_spec;
    };

} // namespace LangCore

#endif // LANGCORE_VERSIONEDTASKSELECTOR_H
