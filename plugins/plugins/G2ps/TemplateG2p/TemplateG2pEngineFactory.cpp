#include "TemplateG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "TemplateG2pTask.h"

namespace LangPlugins
{
    TemplateG2pEngineFactory::TemplateG2pEngineFactory() = default;

    TemplateG2pEngineFactory::~TemplateG2pEngineFactory() = default;

    int TemplateG2pEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    TemplateG2pEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Template::TemplateG2pConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<inferUtil::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        static_assert(std::is_same_v<decltype(result->onnxG2pId), std::string>);
        parser.parse_string_required(result->onnxG2pId, "onnxG2pId");

        static_assert(std::is_same_v<decltype(result->enableOnnxG2p), bool>);
        parser.parse_bool_optional(result->enableOnnxG2p, "enableOnnxG2p");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing TemplateG2p configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    TemplateG2pEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                         const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<TemplateG2pTask>::create(spec);
    }

} // namespace LangPlugins
