#include "RegexTaggerEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "RegexTaggerTask.h"

namespace LangPlugins
{
    RegexTaggerEngineFactory::RegexTaggerEngineFactory() = default;

    RegexTaggerEngineFactory::~RegexTaggerEngineFactory() = default;

    int RegexTaggerEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    RegexTaggerEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Regex::RegexTaggerConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->language), std::string>);
        parser.parse_string_required(result->language, "language");

        static_assert(std::is_same_v<decltype(result->regexEntry), std::vector<Regex::TaggerRegexEntry>>);
        parser.parse_tagger_required(result->regexEntry, "tagger");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing RegexTaggerEngine configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    RegexTaggerEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                         const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<RegexTaggerTask>::create(spec);
    }

} // namespace LangPlugins
