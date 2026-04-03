#ifndef LANGPLUGINS_CHAING2P_STEPS_FALLBACKSTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_FALLBACKSTEP_H

#include "../Core/G2pStep.h"

namespace LangPlugins::ChainG2p
{
    /// FallbackStep - 回退步骤
    ///
    /// 处理转换失败的词
    class FallbackStep : public G2pStep {
    public:
        FallbackStep() = default;
        ~FallbackStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "fallback"; }

    private:
        bool m_useOriginal = true;
        std::string m_defaultPronunciation;
        bool m_markFailed = true;
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_FALLBACKSTEP_H