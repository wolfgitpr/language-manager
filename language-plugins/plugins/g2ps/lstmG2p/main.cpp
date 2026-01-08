#include <LangMgr/Modules/EngineFactoryPlugin.h>

#include "LstmG2pEngineFactory.h"

namespace LangPlugins
{

    class LstmG2pInterpreterPlugin final : public LangMgr::EngineFactoryPlugin {
    public:
        LstmG2pInterpreterPlugin() = default;

        const char *key() const override { return "g2p.model.LstmG2pInference"; }

        LangMgr::NO<LangMgr::EngineFactory> create() override { return LangMgr::NO<LstmG2pEngineFactory>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::LstmG2pInterpreterPlugin)
