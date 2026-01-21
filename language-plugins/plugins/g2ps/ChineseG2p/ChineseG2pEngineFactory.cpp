#include "ChineseG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "ChineseG2pTask.h"

namespace LangPlugins
{
    ChineseG2pEngineFactory::ChineseG2pEngineFactory() = default;

    ChineseG2pEngineFactory::~ChineseG2pEngineFactory() = default;

    int ChineseG2pEngineFactory::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
    ChineseG2pEngineFactory::createConfiguration(const LangMgr::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Chinese::ChineseG2pConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec->as<LangMgr::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<inferUtil::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing ChineseG2p configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
    ChineseG2pEngineFactory::createTask(const LangMgr::ModuleSpec *spec,
                                        const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<ChineseG2pTask>::create(spec);
    }

} // namespace LangPlugins
