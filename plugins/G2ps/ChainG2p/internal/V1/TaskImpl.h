#ifndef LANGPLUGINS_CHAING2P_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_CHAING2P_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <memory>
#include <shared_mutex>
#include <string>

#include <LangCore/Module/Module.h>
#include "../Core/G2pPipeline.h"

namespace LangPlugins::ChainG2p::Internal::V1
{
    /// ChainG2pTaskImpl - ChainG2p 的 Level 1 实现
    /// 使用责任链模式处理 G2p 转换
    class ChainG2pTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit ChainG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~ChainG2pTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        /// 设置 Task 对象
        void setTask(LangCore::Task* task) { m_task = task; }

        /// 获取 Task 对象
        LangCore::Task* task() const { return m_task; }

    private:
        const LangCore::ModuleSpec* m_spec;
        LangCore::Task* m_task = nullptr;
        std::unique_ptr<G2pPipeline> m_pipeline;
        mutable std::shared_mutex m_mutex;
        std::string m_config;
    };

} // namespace LangPlugins::ChainG2p::Internal::V1

#endif // LANGPLUGINS_CHAING2P_INTERNAL_V1_TASKIMPL_H