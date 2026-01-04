#include <LangMgr/Tool/InferenceInterpreterPlugin.h>

#include "RegexSpliterInterpreter.h"

namespace LangPlugins
{

    class RegexSpliterInterpreterPlugin final : public LangMgr::InferenceInterpreterPlugin {
    public:
        RegexSpliterInterpreterPlugin() = default;

        const char *key() const override { return "spliter.regex.RegexSpliterInference"; }

        LangMgr::NO<LangMgr::InferenceInterpreter> create() override {
            return LangMgr::NO<RegexSpliterInterpreter>::create();
        }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::RegexSpliterInterpreterPlugin)
