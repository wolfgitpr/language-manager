#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "TemplateG2pEngineFactory.h"

namespace LangPlugins
{

    class TemplateG2pInterpreterPlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        TemplateG2pInterpreterPlugin() = default;

        const char *key() const override { return "g2p.template.TemplateG2pInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override {
            return LangMgr::NO<TemplateG2pEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::TemplateG2pInterpreterPlugin)
