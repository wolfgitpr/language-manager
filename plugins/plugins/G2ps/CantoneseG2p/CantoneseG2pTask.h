#ifndef LANGPLUGINS_CANTONESEG2PTASK_H
#define LANGPLUGINS_CANTONESEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/CantoneseG2p/1/CantoneseG2pL1.h>

namespace LangPlugins::CantoneseG2p
{
    namespace Cantonese = Api::CantoneseG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class CantoneseG2pTask : public LangCore::Task {
    public:
        explicit CantoneseG2pTask(const LangCore::ModuleSpec *spec);
        ~CantoneseG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::CantoneseG2p

#endif
