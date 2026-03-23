#ifndef LANGPLUGINS_API_CANTONESEG2PAPIL1_H
#define LANGPLUGINS_API_CANTONESEG2PAPIL1_H

#include <vector>

#include <LangCore/Task/G2pTask.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::Api::CantoneseG2p::L1
{

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.CantoneseG2pInference";
    constexpr int API_LEVEL = 1;

    class CantoneseG2pConfiguration : public LangCore::G2pConfiguration {
    public:
        CantoneseG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<InferUtil::VerifyEntry> verifyEntry;
        std::filesystem::path dictPath;
    };

    class CantoneseG2pRuntimeOptions : public LangCore::G2pRuntimeOptions {
    public:
        CantoneseG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class CantoneseG2pInitArgs : public LangCore::G2pInitArgs {
    public:
        CantoneseG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<CantoneseG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::CantoneseG2p::L1

#endif
