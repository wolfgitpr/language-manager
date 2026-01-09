#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "RegexSplitterEngineFactory.h"

namespace LangPlugins
{

    class RegexSpliterInterpreterPlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        RegexSpliterInterpreterPlugin() = default;

        const char *key() const override { return "spliter.regex.RegexSpliterInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override {
            return LangMgr::NO<RegexSplitterEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::RegexSpliterInterpreterPlugin)
