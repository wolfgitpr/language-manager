#include <LangCore/Task/TaskPlugin.h>

#include "CantoneseG2pTask.h"

namespace LangPlugins::CantoneseG2p::V1
{
    class CantoneseG2pEnginePlugin final : public LangCore::TaskPlugin {
    public:
        CantoneseG2pEnginePlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "g2p.template.CantoneseG2pInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<CantoneseG2pTask>::create(spec);
        }
    };

} // namespace LangPlugins::CantoneseG2p::V1

LANGCORE_EXPORT_PLUGIN(LangPlugins::CantoneseG2p::V1::CantoneseG2pEnginePlugin)
