#include <LangCore/Task/TaskFactoryPlugin.h>

#include "TemplateG2pEngineFactory.h"

namespace LangPlugins::TemplateG2p
{

    class TemplateG2pEnginePlugin final : public LangCore::TaskFactoryPlugin {
    public:
        TemplateG2pEnginePlugin() = default;

        const char *key() const override { return "g2p.template.TemplateG2pInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<TemplateG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins::TemplateG2p

LANGCORE_EXPORT_PLUGIN(LangPlugins::TemplateG2p::TemplateG2pEnginePlugin)
