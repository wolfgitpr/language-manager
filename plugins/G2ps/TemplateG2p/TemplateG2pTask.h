#ifndef LANGPLUGINS_TEMPLATEG2PTASK_H
#define LANGPLUGINS_TEMPLATEG2PTASK_H

#include <LangCore/Task/Task.h>

namespace LangPlugins::TemplateG2p::V1
{
    class TemplateG2pTask : public LangCore::Task {
    public:
        explicit TemplateG2pTask(const LangCore::ModuleSpec *spec);
        ~TemplateG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

    private:
        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins::TemplateG2p::V1

#endif // LANGPLUGINS_TEMPLATEG2PTASK_H
