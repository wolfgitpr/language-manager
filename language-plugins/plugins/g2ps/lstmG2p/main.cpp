#include <LangMgr/Tool/InferenceInterpreterPlugin.h>

#include "LstmG2pInterpreter.h"

namespace LangPlugins
{

    class LstmG2pInterpreterPlugin final : public LangMgr::InferenceInterpreterPlugin {
    public:
        LstmG2pInterpreterPlugin() = default;

        const char *key() const override { return "ai.g2p.LstmG2pInference"; }

        LangMgr::NO<LangMgr::InferenceInterpreter> create() override {
            return LangMgr::NO<LstmG2pInterpreter>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::LstmG2pInterpreterPlugin)
