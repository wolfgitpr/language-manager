#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "RegexTaggerEngineFactory.h"

namespace LangPlugins
{

    class RegexTaggerInterpreterPlugin final : public LangMgr::TaskFactoryPlugin {
    public:
        RegexTaggerInterpreterPlugin() = default;

        const char *key() const override { return "tagger.template.RegexTaggerInference"; }

        LangMgr::NO<LangMgr::TaskFactory> create() override {
            return LangMgr::NO<RegexTaggerEngineFactory>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::RegexTaggerInterpreterPlugin)
