#ifndef LANGPLUGINS_TEMPLATEG2PTASK_H
#define LANGPLUGINS_TEMPLATEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/TemplateG2p/1/TemplateG2pL1.h>

namespace LangPlugins
{
    namespace Template = Api::TemplateG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class TemplateG2pTask : public LangCore::Task {
    public:
        explicit TemplateG2pTask(const LangCore::ModuleSpec *spec);
        ~TemplateG2pTask() override;

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

    private:
        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins

#endif //  LANGPLUGINS_TEMPLATEG2PTASK_H
