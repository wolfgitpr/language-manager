#ifndef LANGPLUGINS_TEMPLATECLEANERTASK_H
#define LANGPLUGINS_TEMPLATECLEANERTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/CleanerTask.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <LangCore/Module/Module.h>
#include <memory>

namespace LangPlugins::TemplateCleaner
{
    class TemplateCleanerTask : public LangCore::Task {
    public:
        explicit TemplateCleanerTask(const LangCore::ModuleSpec *spec);
        ~TemplateCleanerTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<TemplateCleanerTask> _manager;
    };

} // namespace LangPlugins::TemplateCleaner

#endif // LANGPLUGINS_TEMPLATECLEANERTASK_H