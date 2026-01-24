#include <LangCore/Task/TaskFactoryPlugin.h>

#include "LstmG2pEngineFactory.h"

namespace LangPlugins
{

    class LstmG2pEnginePlugin final : public LangCore::TaskFactoryPlugin {
    public:
        LstmG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.model.LstmG2pInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override { return LangCore::NO<LstmG2pEngineFactory>::create(); }
    };

} // namespace LangPlugins

LANGCORE_EXPORT_PLUGIN(LangPlugins::LstmG2pEnginePlugin)
