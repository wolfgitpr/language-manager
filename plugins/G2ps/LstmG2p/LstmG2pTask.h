#ifndef LANGPLUGINS_LSTMG2PTASK_H
#define LANGPLUGINS_LSTMG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::LstmG2p
{
    class LstmG2pTask : public LangCore::Task {
    public:
        explicit LstmG2pTask(const LangCore::ModuleSpec *spec);
        ~LstmG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<LstmG2pTask> _manager;
    };

} // namespace LangPlugins::LstmG2p

#endif // LANGPLUGINS_LSTMG2PTASK_H
