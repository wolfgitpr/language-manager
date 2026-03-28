#include <LangCore/Task/TaskPlugin.h>

#include "TemplateG2pTask.h"

namespace LangPlugins::TemplateG2p::V1
{

    class TemplateG2pEnginePlugin final : public LangCore::TaskPlugin {
    public:
        TemplateG2pEnginePlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "g2p.template.TemplateG2pInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<TemplateG2pTask>::create(spec);
        }
    };

} // namespace LangPlugins::TemplateG2p::V1

LANGCORE_EXPORT_PLUGIN(LangPlugins::TemplateG2p::V1::TemplateG2pEnginePlugin)
