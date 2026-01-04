#include "RegexSpliterInterpreter.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "RegexSpliterInference.h"

namespace LangPlugins
{
    RegexSpliterInterpreter::RegexSpliterInterpreter() = default;

    RegexSpliterInterpreter::~RegexSpliterInterpreter() = default;

    int RegexSpliterInterpreter::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceSchema>>
    RegexSpliterInterpreter::createSchema(const LangMgr::InferenceSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createSchema: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Regex::RegexSpliterSchema>::create();

        // Collect all the errors and return to user
        if (const inferUtil::ErrorCollector ec; ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration schema"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceConfiguration>>
    RegexSpliterInterpreter::createConfiguration(const LangMgr::InferenceSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        const auto &config = spec->manifestConfiguration();
        auto result = LangMgr::NO<Regex::RegexSpliterConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec, &ec);

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

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceImportOptions>>
    RegexSpliterInterpreter::createImportOptions(const LangMgr::InferenceSpec *spec,
                                                 const LangMgr::JsonValue &options) const {
        if (!options.isObject()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                "error parsing duration import options: import options JSON should be an object",
            };
        }
        const auto &obj = options.toObject();
        auto result = LangMgr::NO<Regex::RegexSpliterImportOptions>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ImportOptionsParser parser(spec, &ec, obj);
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
    RegexSpliterInterpreter::createInference(const LangMgr::InferenceSpec *spec,
                                             const LangMgr::NO<LangMgr::InferenceImportOptions> &importOptions,
                                             const LangMgr::NO<LangMgr::InferenceRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<RegexSpliterInference>::create(spec);
    }

} // namespace LangPlugins
