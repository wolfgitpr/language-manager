#ifndef LANG_PLUGINS_REGEXTAGGERTASK_H
#define LANG_PLUGINS_REGEXTAGGERTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>

namespace LangPlugins::TemplateTagger
{
    class TemplateTaggerTask : public LangCore::Task {
    public:
        explicit TemplateTaggerTask(const LangCore::ModuleSpec *spec);
        ~TemplateTaggerTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::TemplateTagger

#endif //  LANG_PLUGINS_REGEXTAGGERTASK_H
