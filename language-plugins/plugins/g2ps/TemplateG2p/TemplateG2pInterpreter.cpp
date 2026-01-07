#include "TemplateG2pInterpreter.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "TemplateG2pInference.h"

namespace LangPlugins
{
    TemplateG2pInterpreter::TemplateG2pInterpreter() = default;

    TemplateG2pInterpreter::~TemplateG2pInterpreter() = default;

    int TemplateG2pInterpreter::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceConfiguration>>
    TemplateG2pInterpreter::createConfiguration(const LangMgr::InferenceSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Template::TemplateG2pConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec, &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<Template::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        static_assert(std::is_same_v<decltype(result->onnxInferenceId), std::string>);
        parser.parse_string_required(result->onnxInferenceId, "onnxInferenceId");

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
    TemplateG2pInterpreter::createInference(const LangMgr::InferenceSpec *spec,
                                            const LangMgr::NO<LangMgr::InferenceRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<TemplateG2pInference>::create(spec);
    }

} // namespace LangPlugins
