#include <LangCore/Task/TaskPlugin.h>

#include "MandarinG2pTask.h"

namespace LangPlugins::MandarinG2p::V1
{
    class MandarinG2pEnginePlugin final : public LangCore::TaskPlugin {
    public:
        MandarinG2pEnginePlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "g2p.template.MandarinG2pInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<MandarinG2pTask>::create(spec);
        }
    };

} // namespace LangPlugins::MandarinG2p::V1

LANGCORE_EXPORT_PLUGIN(LangPlugins::MandarinG2p::V1::MandarinG2pEnginePlugin)
