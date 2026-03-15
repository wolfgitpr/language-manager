#include <LangCore/Task/TaskFactoryPlugin.h>

#include "RegexSplitterEngineFactory.h"

namespace LangPlugins::RegexSplitter
{

    class RegexSplitterInterpreterPlugin final : public LangCore::TaskFactoryPlugin {
    public:
        RegexSplitterInterpreterPlugin() = default;

        const char *key() const override { return "splitter.regex.RegexSplitterInference"; }

        LangCore::NO<LangCore::TaskFactory> create() override {
            return LangCore::NO<RegexSplitterEngineFactory>::create();
        }
    };

} // namespace LangPlugins::RegexSplitter

LANGCORE_EXPORT_PLUGIN(LangPlugins::RegexSplitter::RegexSplitterInterpreterPlugin)
