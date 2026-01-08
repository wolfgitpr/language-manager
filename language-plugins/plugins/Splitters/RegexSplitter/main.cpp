#include <LangMgr/Modules/EngineFactoryPlugin.h>

#include "RegexSplitterEngineFactory.h"

namespace LangPlugins
{

    class RegexSpliterInterpreterPlugin final : public LangMgr::EngineFactoryPlugin {
    public:
        RegexSpliterInterpreterPlugin() = default;

        const char *key() const override { return "spliter.regex.RegexSpliterInference"; }

        LangMgr::NO<LangMgr::EngineFactory> create() override {
            return LangMgr::NO<RegexSplitterEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::RegexSpliterInterpreterPlugin)
