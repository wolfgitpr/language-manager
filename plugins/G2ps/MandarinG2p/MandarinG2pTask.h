#ifndef LANGPLUGINS_MANDARING2PTASK_H
#define LANGPLUGINS_MANDARING2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::MandarinG2p
{
    class MandarinG2pTask : public LangCore::Task {
    public:
        explicit MandarinG2pTask(const LangCore::ModuleSpec *spec);
        ~MandarinG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

    private:
        LangCore::VersionedTaskManager<MandarinG2pTask> _manager;
    };

} // namespace LangPlugins::MandarinG2p

#endif // LANGPLUGINS_MANDARING2PTASK_H
