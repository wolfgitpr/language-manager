#ifndef LANGPLUGINS_CANTONESEG2PTASK_H
#define LANGPLUGINS_CANTONESEG2PTASK_H

#include <LangCore/Module/Module.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <memory>

namespace LangPlugins::CantoneseG2p
{
    class CantoneseG2pTask : public LangCore::Task {
    public:
        explicit CantoneseG2pTask(const LangCore::ModuleSpec *spec);
        ~CantoneseG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

    private:
        LangCore::VersionedTaskManager<CantoneseG2pTask> _manager;
    };

} // namespace LangPlugins::CantoneseG2p

#endif // LANGPLUGINS_CANTONESEG2PTASK_H
