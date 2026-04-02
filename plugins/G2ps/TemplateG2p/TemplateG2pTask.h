#ifndef LANGPLUGINS_TEMPLATEG2PTASK_H
#define LANGPLUGINS_TEMPLATEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::TemplateG2p
{
    class TemplateG2pTask : public LangCore::Task {
    public:
        explicit TemplateG2pTask(const LangCore::ModuleSpec *spec);
        ~TemplateG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<TemplateG2pTask> _manager;
    };

} // namespace LangPlugins::TemplateG2p

#endif // LANGPLUGINS_TEMPLATEG2PTASK_H
