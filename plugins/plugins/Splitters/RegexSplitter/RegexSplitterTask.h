#ifndef LANG_PLUGINS_REGEXTAGGERTASK_H
#define LANG_PLUGINS_REGEXTAGGERTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/Splitters/RegexSplitter/1/RegexSplitterL1.h>

namespace LangPlugins::RegexSplitter
{
    namespace Regex = Api::RegexSplitter::L1;
    namespace Onnx = Api::Onnx::L1;

    class RegexSplitterTask : public LangCore::Task {
    public:
        explicit RegexSplitterTask(const LangCore::ModuleSpec *spec);
        ~RegexSplitterTask() override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;
        LangCore::Expected<void> startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                            const StartAsyncCallback &callback) override;
        bool stop() override;

        LangCore::NO<LangCore::TaskResult> result() const override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::RegexSplitter

#endif //  LANG_PLUGINS_REGEXTAGGERTASK_H
