#include <LangCore/Task/TaskPlugin.h>

#include "RegexSplitterTask.h"

namespace LangPlugins::RegexSplitter
{

    class RegexSplitterPlugin final : public LangCore::TaskPlugin {
    public:
        RegexSplitterPlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "splitter.regex.RegexSplitterInference"; }

        LangCore::Expected<LangCore::NO<LangCore::Task>> createTask(const LangCore::ModuleSpec *spec) override {
            return LangCore::NO<RegexSplitterTask>::create(spec);
        }
    };

} // namespace LangPlugins::RegexSplitter

LANGCORE_EXPORT_PLUGIN(LangPlugins::RegexSplitter::RegexSplitterPlugin)
