#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "RegexSplitterEngineFactory.h"

namespace LangPlugins
{

    class RegexSplitterInterpreterPlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        RegexSplitterInterpreterPlugin() = default;

        const char *key() const override { return "splitter.regex.RegexSplitterInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override {
            return LangMgr::NO<RegexSplitterEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::RegexSplitterInterpreterPlugin)
