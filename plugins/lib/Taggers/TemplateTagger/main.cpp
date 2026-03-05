#include <LangCore/Task/TaskFactoryPlugin.h>

#include "TemplateTaggerEngineFactory.h"

namespace LangPlugins::TemplateTagger
{

    class RegexTaggerInterpreterPlugin final : public LangCore::TaskFactoryPlugin {
    public:
        RegexTaggerInterpreterPlugin() = default;

        const char *key() const override { return "tagger.template.TemplateTaggerInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<TemplateTaggerEngineFactory>::create();
        }
    };

} // namespace LangPlugins::TemplateTagger

LANGCORE_EXPORT_PLUGIN(LangPlugins::TemplateTagger::RegexTaggerInterpreterPlugin)
