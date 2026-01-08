#ifndef LANG_PLUGINS_REGEXSPLITTERTASK_H
#define LANG_PLUGINS_REGEXSPLITTERTASK_H

#include <LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Inferences/RegexSplitter/1/RegexSplitterL1.h>

namespace LangPlugins
{
    namespace Regex = Api::RegexSplitter::L1;

    class RegexSplitterTask : public LangMgr::Task {
    public:
        explicit RegexSplitterTask(const LangMgr::ModuleDefinition *definition);
        ~RegexSplitterTask() override;

        LangMgr::Expected<void> initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) override;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
        start(const LangMgr::NO<LangMgr::TaskStartInput> &input) override;
        LangMgr::Expected<void> startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                           const StartAsyncCallback &callback) override;
        bool stop() override;

        LangMgr::NO<LangMgr::TaskResult> result() const override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
} // namespace LangPlugins

#endif // LANG_PLUGINS_REGEXSPLITTERTASK_H
