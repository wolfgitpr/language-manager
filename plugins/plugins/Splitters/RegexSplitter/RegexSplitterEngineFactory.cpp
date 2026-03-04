#include "RegexSplitterEngineFactory.h"

#include <stdcorelib/str.h>

#include <InferUtil/Parser.h>

#include "RegexSplitterTask.h"

namespace LangPlugins::RegexSplitter
{
    RegexSplitterEngineFactory::RegexSplitterEngineFactory() = default;

    RegexSplitterEngineFactory::~RegexSplitterEngineFactory() = default;

    int RegexSplitterEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    RegexSplitterEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Regex::RegexSplitterConfiguration>::create();

        // Collect all the errors and return to user
        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->regexes), std::vector<std::string>>);
        parser.parse_stringVec_required(result->regexes, "regexes");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing RegexSplitterEngine configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    RegexSplitterEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                           const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<RegexSplitterTask>::create(spec);
    }

} // namespace LangPlugins::RegexSplitter
