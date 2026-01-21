#ifndef LANG_PLUGINS_CHINESEG2PTASK_H
#define LANG_PLUGINS_CHINESEG2PTASK_H

#include <LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/ChineseG2p/1/ChineseG2pL1.h>

namespace LangPlugins
{
    namespace Chinese = Api::ChineseG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class ChineseG2pTask : public LangMgr::Task {
    public:
        explicit ChineseG2pTask(const LangMgr::ModuleSpec *spec);
        ~ChineseG2pTask() override;

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

#endif //  LANG_PLUGINS_CHINESEG2PTASK_H
