#ifndef LANGPLUGINS_CHINESEG2PTASK_H
#define LANGPLUGINS_CHINESEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/MandarinG2p/1/MandarinG2pL1.h>

namespace LangPlugins::MandarinG2p
{
    namespace Mandarin = Api::MandarinG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class MandarinG2pTask : public LangCore::Task {
    public:
        explicit MandarinG2pTask(const LangCore::ModuleSpec *spec);
        ~MandarinG2pTask() override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::MandarinG2p

#endif //  LANGPLUGINS_CHINESEG2PTASK_H
