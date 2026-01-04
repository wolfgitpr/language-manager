#include <LangMgr/Tool/InferenceInterpreterPlugin.h>

#include "TemplateG2pInterpreter.h"

namespace LangPlugins
{

    class TemplateG2pInterpreterPlugin final : public LangMgr::InferenceInterpreterPlugin {
    public:
        TemplateG2pInterpreterPlugin() = default;

        const char *key() const override { return "g2p.template.TemplateInference"; }

        LangMgr::NO<LangMgr::InferenceInterpreter> create() override {
            return LangMgr::NO<TemplateG2pInterpreter>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::TemplateG2pInterpreterPlugin)
