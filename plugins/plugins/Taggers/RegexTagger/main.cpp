#include <LangCore/Task/TaskFactoryPlugin.h>

#include "RegexTaggerEngineFactory.h"

namespace LangPlugins
{

    class RegexTaggerInterpreterPlugin final : public LangCore::TaskFactoryPlugin {
    public:
        RegexTaggerInterpreterPlugin() = default;

        const char *key() const override { return "tagger.template.RegexTaggerInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<RegexTaggerEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGCORE_EXPORT_PLUGIN(LangPlugins::RegexTaggerInterpreterPlugin)
