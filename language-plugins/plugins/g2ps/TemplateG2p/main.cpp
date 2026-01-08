#include <LangMgr/Modules/EngineFactoryPlugin.h>

#include "TemplateG2pEngineFactory.h"

namespace LangPlugins
{

    class TemplateG2pInterpreterPlugin final : public LangMgr::EngineFactoryPlugin {
    public:
        TemplateG2pInterpreterPlugin() = default;

        const char *key() const override { return "g2p.template.TemplateInference"; }

        LangMgr::NO<LangMgr::EngineFactory> create() override {
            return LangMgr::NO<TemplateG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::TemplateG2pInterpreterPlugin)
