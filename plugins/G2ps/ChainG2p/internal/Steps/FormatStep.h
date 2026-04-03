#ifndef LANGPLUGINS_CHAING2P_STEPS_FORMATSTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_FORMATSTEP_H

#include "../Core/G2pStep.h"

namespace LangPlugins::ChainG2p
{
    /// FormatStep - 格式化步骤
    ///
    /// 格式化输出结果
    class FormatStep : public G2pStep {
    public:
        FormatStep() = default;
        ~FormatStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "format"; }

    private:
        bool m_stripTrailingSpace = false;
        bool m_normalizeTones = false;
        bool m_addSpaceBetweenPhones = false;

        static std::string stripTrailingSpace(const std::string &str);
        static std::string addSpaceBetweenPhones(const std::string &str);
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_FORMATSTEP_H