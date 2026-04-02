#ifndef LANGPLUGINS_TEMPLATETAGGERTASK_H
#define LANGPLUGINS_TEMPLATETAGGERTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::TemplateTagger
{
    class TemplateTaggerTask : public LangCore::Task {
    public:
        explicit TemplateTaggerTask(const LangCore::ModuleSpec *spec);
        ~TemplateTaggerTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<TemplateTaggerTask> _manager;
    };

} // namespace LangPlugins::TemplateTagger

#endif // LANGPLUGINS_TEMPLATETAGGERTASK_H
