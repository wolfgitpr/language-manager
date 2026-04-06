#ifndef LANGCORE_VERSIONEDTASKIMPLBASE_H
#define LANGCORE_VERSIONEDTASKIMPLBASE_H

#include <LangCore/Task/Task.h>
#include <memory>

namespace LangCore
{
    /// VersionedTaskImplBase - 多版本任务实现的基类接口
    ///
    /// 这个抽象基类定义了所有版本实现必须遵循的统一接口。
    /// 它提供了一个稳定的契约，允许插件支持多个 API Level 的实现。
    ///
    /// 设计原则：
    /// - 只定义接口，不包含实现细节
    /// - 支持多版本扩展
    /// - 类型安全的任务执行
    class VersionedTaskImplBase {
    public:
        virtual ~VersionedTaskImplBase() = default;

        /// 初始化任务实现
        /// @return 成功返回 Expected<void>::success()，失败返回错误信息
        virtual Expected<void> initialize() = 0;

        /// 执行任务
        /// @param input 任务输入数据
        /// @return 成功返回任务结果，失败返回错误信息
        virtual Expected<NO<TaskResult>>
        start(const NO<TaskInput> &input) = 0;

        /// 获取当前配置
        /// @return JSON 格式的配置字符串
        virtual std::string getConfig() const = 0;
    };

} // namespace LangCore

#endif // LANGCORE_VERSIONEDTASKIMPLBASE_H