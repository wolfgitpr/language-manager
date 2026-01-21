#include "RegexTaggerEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "RegexTaggerTask.h"

namespace LangPlugins
{
    RegexTaggerEngineFactory::RegexTaggerEngineFactory() = default;

    RegexTaggerEngineFactory::~RegexTaggerEngineFactory() = default;

    int RegexTaggerEngineFactory::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
    RegexTaggerEngineFactory::createConfiguration(const LangMgr::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Regex::RegexTaggerConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangMgr::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->language), std::string>);
        parser.parse_string_required(result->language, "language");

        static_assert(std::is_same_v<decltype(result->regexEntry), std::vector<Regex::TaggerRegexEntry>>);
        parser.parse_tagger_required(result->regexEntry, "tagger");

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing RegexTaggerEngine configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
    RegexTaggerEngineFactory::createTask(const LangMgr::ModuleSpec *spec,
                                         const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<RegexTaggerTask>::create(spec);
    }

} // namespace LangPlugins
