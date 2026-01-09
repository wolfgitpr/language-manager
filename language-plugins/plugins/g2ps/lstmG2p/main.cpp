#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "LstmG2pEngineFactory.h"

namespace LangPlugins
{

    class LstmG2pInterpreterPlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        LstmG2pInterpreterPlugin() = default;

        const char *key() const override { return "g2p.model.LstmG2pInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override { return LangMgr::NO<LstmG2pEngineFactory>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::LstmG2pInterpreterPlugin)
