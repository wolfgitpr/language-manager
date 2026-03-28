#ifndef LANG_PLUGINS_REGEXTAGGERTASK_H
#define LANG_PLUGINS_REGEXTAGGERTASK_H

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

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::RegexSplitter::V1

#endif //  LANG_PLUGINS_REGEXTAGGERTASK_H
