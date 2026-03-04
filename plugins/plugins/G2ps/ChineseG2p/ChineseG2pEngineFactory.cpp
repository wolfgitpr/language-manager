#include "ChineseG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "ChineseG2pTask.h"

namespace LangPlugins::ChineseG2p
{
    ChineseG2pEngineFactory::ChineseG2pEngineFactory() = default;

    ChineseG2pEngineFactory::~ChineseG2pEngineFactory() = default;

    int ChineseG2pEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    ChineseG2pEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Chinese::ChineseG2pConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<inferUtil::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing ChineseG2p configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    ChineseG2pEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                        const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<ChineseG2pTask>::create(spec);
    }

} // namespace LangPlugins::ChineseG2p
