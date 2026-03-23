#include <LangCore/Task/TaskFactoryPlugin.h>

#include "CantoneseG2pEngineFactory.h"

namespace LangPlugins::CantoneseG2p
{

    class CantoneseG2pEnginePlugin final : public LangCore::TaskFactoryPlugin {
    public:
        CantoneseG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.template.CantoneseG2pInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<CantoneseG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins::CantoneseG2p

LANGCORE_EXPORT_PLUGIN(LangPlugins::CantoneseG2p::CantoneseG2pEnginePlugin)
