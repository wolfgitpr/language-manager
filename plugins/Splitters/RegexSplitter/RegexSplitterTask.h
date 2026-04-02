#ifndef LANGPLUGINS_REGEXSPLITTERTASK_H
#define LANGPLUGINS_REGEXSPLITTERTASK_H

#include <LangCore/Task/Task.h>
#include <LangCore/Task/VersionedTaskManager.h>
#include <memory>

namespace LangPlugins::RegexSplitter
{
    class RegexSplitterTask : public LangCore::Task {
    public:
        explicit RegexSplitterTask(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;
        LangCore::Expected<void> setConfig(const std::string &config) override;

    private:
        LangCore::VersionedTaskManager<RegexSplitterTask> _manager;
    };

} // namespace LangPlugins::RegexSplitter

#endif // LANGPLUGINS_REGEXSPLITTERTASK_H
