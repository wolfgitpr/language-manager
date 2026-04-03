#ifndef LANGPLUGINS_CHAING2PTASK_H
#define LANGPLUGINS_CHAING2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::ChainG2p
{
    class ChainG2pTask : public LangCore::Task {
    public:
        explicit ChainG2pTask(const LangCore::ModuleSpec *spec);
        ~ChainG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<ChainG2pTask> _manager;
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2PTASK_H