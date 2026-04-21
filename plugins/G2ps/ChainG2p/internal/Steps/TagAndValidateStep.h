#ifndef LANGPLUGINS_CHAING2P_STEPS_TAGANDVALIDATESTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_TAGANDVALIDATESTEP_H

#include "../Core/G2pStep.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <vector>
#include <string>

namespace LangPlugins::ChainG2p
{
    /// TagAndValidateStep - 标记和验证步骤
    ///
    /// 使用正则表达式对输入词进行分类，决定 copy/convert 模式
    class TagAndValidateStep : public G2pStep {
    public:
        TagAndValidateStep() = default;
        ~TagAndValidateStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "tagAndValidate"; }

        void cleanup() override {}

    private:
        struct VerifyEntry {
            std::string type;
            std::vector<std::string> value;
            std::string mode;
        };

        std::vector<VerifyEntry> m_verifyEntries;
        const LangCore::ModuleSpec* m_spec = nullptr;

        bool verifyWord(const std::string &word, std::string &mode) const;
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_TAGANDVALIDATESTEP_H