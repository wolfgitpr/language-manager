#include <LangCore/Task/TaskPlugin.h>

#include "TemplateTaggerTask.h"

namespace LangPlugins::TemplateTagger
{

    class RegexTaggerInterpreterPlugin final : public LangCore::TaskPlugin {
    public:
        RegexTaggerInterpreterPlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "tagger.template.TemplateTaggerInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<TemplateTaggerTask>::create(spec);
        }
    };

} // namespace LangPlugins::TemplateTagger

LANGCORE_EXPORT_PLUGIN(LangPlugins::TemplateTagger::RegexTaggerInterpreterPlugin)
