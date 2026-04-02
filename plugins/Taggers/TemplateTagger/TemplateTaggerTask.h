#ifndef LANGPLUGINS_TEMPLATETAGGERTASK_H
#define LANGPLUGINS_TEMPLATETAGGERTASK_H

#include <LangCore/Task/Task.h>

namespace LangPlugins::TemplateTagger::V1
{
    class TemplateTaggerTask : public LangCore::Task {
    public:
        explicit TemplateTaggerTask(const LangCore::ModuleSpec *spec);
        ~TemplateTaggerTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::TemplateTagger::V1

#endif // LANGPLUGINS_TEMPLATETAGGERTASK_H
