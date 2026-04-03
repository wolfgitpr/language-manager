#ifndef LANGPLUGINS_CHAING2P_STEPS_CLEANSTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_CLEANSTEP_H

#include "../Core/G2pStep.h"
#include <vector>

namespace LangPlugins::ChainG2p
{
    /// CleanStep - 清洗步骤
    ///
    /// 清洗文本，去除不需要的字符或格式
    class CleanStep : public G2pStep {
    public:
        CleanStep() = default;
        ~CleanStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "clean"; }

    private:
        bool m_trim = false;
        bool m_lowercase = false;
        bool m_uppercase = false;
        bool m_removeSymbols = false;
        bool m_removeNumbers = false;

        static std::string trimString(const std::string &str);
        static std::string toLowercase(const std::string &str);
        static std::string toUppercase(const std::string &str);
        static std::string removeSymbols(const std::string &str);
        static std::string removeNumbers(const std::string &str);
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_CLEANSTEP_H