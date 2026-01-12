#include "RegexSplitterEngineFactory.h"
#include "RegexSplitterTask.h"

#include <inferutil/Parser.h>
#include <stdcorelib/str.h>

#include <LangMgr/Module/SplitterModule.h>

namespace LangPlugins
{
    RegexSplitterEngineFactory::RegexSplitterEngineFactory() = default;

    RegexSplitterEngineFactory::~RegexSplitterEngineFactory() = default;

    int RegexSplitterEngineFactory::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
    RegexSplitterEngineFactory::createConfiguration(const LangMgr::ModuleDefinition *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceDefinition is nullptr",
            };
        }

        const auto &config = spec->manifestConfiguration();
        auto result = LangMgr::NO<Regex::RegexSplitterConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangMgr::SplitterDefinition>(), &ec);

        // [REQUIRED] regex, string (json value is string)
        {
            static_assert(std::is_same_v<decltype(result->regexStr), std::string>);
            parser.parse_string_required(result->regexStr, "regex");
        } // regex

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
    RegexSplitterEngineFactory::createTask(const LangMgr::ModuleDefinition *spec,
                                           const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<RegexSplitterTask>::create(spec->as<LangMgr::SplitterDefinition>());
    }

} // namespace LangPlugins
