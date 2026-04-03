#ifndef LANGPLUGINS_CHAING2P_STEPS_DICTSTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_DICTSTEP_H

#include "../Core/G2pStep.h"
#include <LangCore/Support/PhonemeDict.h>
#include <memory>

namespace LangPlugins::ChainG2p
{
    /// DictStep - 字典查找步骤
    ///
    /// 从字典中查找发音
    class DictStep : public G2pStep {
    public:
        DictStep() = default;
        ~DictStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "dict"; }

        void cleanup() override;

    private:
        bool m_enabled = true;
        std::filesystem::path m_dictPath;
        LangCore::PhonemeDict m_phonemeDict;

        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_DICTSTEP_H