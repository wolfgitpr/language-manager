#include "TemplateG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "TemplateG2pTask.h"

namespace LangPlugins
{
    TemplateG2pEngineFactory::TemplateG2pEngineFactory() = default;

    TemplateG2pEngineFactory::~TemplateG2pEngineFactory() = default;

    int TemplateG2pEngineFactory::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
    TemplateG2pEngineFactory::createConfiguration(const LangMgr::ModuleSpec *spec) const {
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
        inferUtil::ConfigurationParser parser(spec->as<LangMgr::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<Template::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        static_assert(std::is_same_v<decltype(result->onnxG2pId), std::string>);
        parser.parse_string_required(result->onnxG2pId, "onnxG2pId");

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
    TemplateG2pEngineFactory::createTask(const LangMgr::ModuleSpec *spec,
                                         const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<TemplateG2pTask>::create(spec);
    }

} // namespace LangPlugins
