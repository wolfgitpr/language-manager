#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "ChineseG2pEngineFactory.h"

namespace LangPlugins
{

    class ChineseG2pEnginePlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        ChineseG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.template.ChineseG2pInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override { return LangMgr::NO<ChineseG2pEngineFactory>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::ChineseG2pEnginePlugin)
