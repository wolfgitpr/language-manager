#include <LangMgr/Tool/InferenceInterpreterPlugin.h>

#include "EnglishInterpreter.h"

namespace LangPlugins
{

    class EnglishInterpreterPlugin final : public LangMgr::InferenceInterpreterPlugin {
    public:
        EnglishInterpreterPlugin() = default;

        const char *key() const override { return "ai.g2p.EnglishInference"; }

        LangMgr::NO<LangMgr::InferenceInterpreter> create() override {
            return LangMgr::NO<EnglishInterpreter>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::EnglishInterpreterPlugin)
