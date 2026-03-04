#ifndef LANGPLUGINS_CHINESEG2PTASK_H
#define LANGPLUGINS_CHINESEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/ChineseG2p/1/ChineseG2pL1.h>

namespace LangPlugins::ChineseG2p
{
    namespace Chinese = Api::ChineseG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class ChineseG2pTask : public LangCore::Task {
    public:
        explicit ChineseG2pTask(const LangCore::ModuleSpec *spec);
        ~ChineseG2pTask() override;

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

} // namespace LangPlugins

#endif //  LANGPLUGINS_CHINESEG2PTASK_H
