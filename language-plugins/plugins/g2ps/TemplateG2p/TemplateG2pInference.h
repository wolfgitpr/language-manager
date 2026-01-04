#ifndef LANG_PLUGINS_TEMPLATEG2PINFERENCE_H
#define LANG_PLUGINS_TEMPLATEG2PINFERENCE_H

#include <LangMgr/Tool/Inference.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/OnnxDriverApi.h>
#include <LangPlugins/Api/Inferences/Common/1/CommonApiL1.h>
#include <LangPlugins/Api/Inferences/TemplateG2p/1/TemplateG2pL1.h>
#include <stdcorelib/str.h>

namespace LangPlugins
{
    namespace Template = Api::TemplateG2p::L1;
    namespace Common = Api::Common::L1;
    namespace Onnx = Api::Onnx;

    class TemplateG2pInference : public LangMgr::Inference {
    public:
        explicit TemplateG2pInference(const LangMgr::InferenceSpec *spec);
        ~TemplateG2pInference() override;

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

    private:
        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins

#endif // LANG_PLUGINS_TEMPLATEG2PINFERENCE_H
