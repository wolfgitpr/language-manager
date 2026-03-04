#include "TemplateTaggerEngineFactory.h"

#include <stdcorelib/str.h>

#include <InferUtil/Parser.h>

#include "TemplateTaggerTask.h"

namespace LangPlugins::TemplateTagger
{
    TemplateTaggerEngineFactory::TemplateTaggerEngineFactory() = default;

    TemplateTaggerEngineFactory::~TemplateTaggerEngineFactory() = default;

    int TemplateTaggerEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    TemplateTaggerEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "Fatal in createConfiguration: InferenceSpec is nullptr.",
            };
        }

        auto result = LangCore::NO<Regex::TemplateTaggerConfiguration>::create();

        // Collect all the errors and return to user
        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->language), std::string>);
        parser.parse_string_required(result->language, "language");

        static_assert(std::is_same_v<decltype(result->taggerUtilEntry), std::vector<Regex::TaggerUtilEntry>>);
        parser.parse_tagger_required(result->taggerUtilEntry, "tagger");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing RegexTaggerEngine configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    TemplateTaggerEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                            const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<TemplateTaggerTask>::create(spec);
    }

} // namespace LangPlugins::TemplateTagger
