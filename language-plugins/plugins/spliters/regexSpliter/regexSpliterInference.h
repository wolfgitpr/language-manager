#ifndef LANG_PLUGINS_REGEXSPLITERINFERENCE_H
#define LANG_PLUGINS_REGEXSPLITERINFERENCE_H

#include <LangMgr/Tool/Inference.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Inferences/RegexSpliter/1/RegexSpliterL1.h>

namespace LangPlugins
{
    namespace Regex = Api::RegexSpliter::L1;

    class RegexSpliterInference : public LangMgr::Inference {
    public:
        explicit RegexSpliterInference(const LangMgr::InferenceSpec *spec);
        ~RegexSpliterInference() override;

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

#endif // LANG_PLUGINS_REGEXSPLITERINFERENCE_H
