#include <LangCore/Task/TaskFactoryPlugin.h>

#include "MandarinG2pEngineFactory.h"

namespace LangPlugins::MandarinG2p
{

    class MandarinG2pEnginePlugin final : public LangCore::TaskFactoryPlugin {
    public:
        MandarinG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.template.MandarinG2pInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<MandarinG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins::MandarinG2p

LANGCORE_EXPORT_PLUGIN(LangPlugins::MandarinG2p::MandarinG2pEnginePlugin)
