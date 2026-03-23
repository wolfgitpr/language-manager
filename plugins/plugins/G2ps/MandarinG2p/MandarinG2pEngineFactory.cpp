#include "MandarinG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <InferUtil/Parser.h>

#include "MandarinG2pTask.h"

namespace LangPlugins::MandarinG2p
{
    MandarinG2pEngineFactory::MandarinG2pEngineFactory() = default;

    MandarinG2pEngineFactory::~MandarinG2pEngineFactory() = default;

    int MandarinG2pEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    MandarinG2pEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Mandarin::MandarinG2pConfiguration>::create();

        // Collect all the errors and return to user
        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec->as<LangCore::ModuleSpec>(), &ec);

        static_assert(std::is_same_v<decltype(result->verifyEntry), std::vector<InferUtil::VerifyEntry>>);
        parser.parse_verify_required(result->verifyEntry, "verify");

        static_assert(std::is_same_v<decltype(result->dictPath), std::filesystem::path>);
        parser.parse_path_required(result->dictPath, "dictPath");

        if (ec.hasErrors()) {
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing MandarinG2p configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    MandarinG2pEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                        const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<MandarinG2pTask>::create(spec);
    }

} // namespace LangPlugins::MandarinG2p
