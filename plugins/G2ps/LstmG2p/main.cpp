#include <LangCore/Task/TaskPlugin.h>

#include "LstmG2pTask.h"

namespace LangPlugins::LstmG2p
{
    class LstmG2pEnginePlugin final : public LangCore::TaskPlugin {
    public:
        LstmG2pEnginePlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "g2p.model.LstmG2pInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<LstmG2pTask>::create(spec);
        }
    };

} // namespace LangPlugins::LstmG2p

LANGCORE_EXPORT_PLUGIN(LangPlugins::LstmG2p::LstmG2pEnginePlugin)
