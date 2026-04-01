#ifndef LANGPLUGINS_REGEXSPLITTERTASK_H
#define LANGPLUGINS_REGEXSPLITTERTASK_H

#include <LangCore/Task/Task.h>

namespace LangPlugins::RegexSplitter::V1
{
    class RegexSplitterTask : public LangCore::Task {
    public:
        explicit RegexSplitterTask(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        LangCore::Expected<void> updateConfig(const std::string &config);

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::RegexSplitter::V1

#endif // LANGPLUGINS_REGEXSPLITTERTASK_H
