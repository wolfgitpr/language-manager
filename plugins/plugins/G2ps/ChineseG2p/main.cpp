#include <LangCore/Task/TaskFactoryPlugin.h>

#include "ChineseG2pEngineFactory.h"

namespace LangPlugins::ChineseG2p
{

    class ChineseG2pEnginePlugin final : public LangCore::TaskFactoryPlugin {
    public:
        ChineseG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.template.ChineseG2pInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<ChineseG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins::ChineseG2p

LANGCORE_EXPORT_PLUGIN(LangPlugins::ChineseG2p::ChineseG2pEnginePlugin)
