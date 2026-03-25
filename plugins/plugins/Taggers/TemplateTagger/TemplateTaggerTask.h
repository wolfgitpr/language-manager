#ifndef LANG_PLUGINS_REGEXTAGGERTASK_H
#define LANG_PLUGINS_REGEXTAGGERTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/Taggers/TemplateTagger/1/TemplateTaggerL1.h>

namespace LangPlugins::TemplateTagger
{
    namespace Regex = Api::TemplateTagger::L1;
    namespace Onnx = Api::Onnx::L1;

    class TemplateTaggerTask : public LangCore::Task {
    public:
        explicit TemplateTaggerTask(const LangCore::ModuleSpec *spec);
        ~TemplateTaggerTask() override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::TemplateTagger

#endif //  LANG_PLUGINS_REGEXTAGGERTASK_H
