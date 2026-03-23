#ifndef LANGPLUGINS_API_MANDARING2PAPIL1_H
#define LANGPLUGINS_API_MANDARING2PAPIL1_H

#include <vector>

#include <LangCore/Task/G2pTask.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::Api::MandarinG2p::L1
{

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.MandarinG2pInference";
    constexpr int API_LEVEL = 1;

    class MandarinG2pConfiguration : public LangCore::G2pConfiguration {
    public:
        MandarinG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<InferUtil::VerifyEntry> verifyEntry;
        std::filesystem::path dictPath;
    };

    class MandarinG2pRuntimeOptions : public LangCore::G2pRuntimeOptions {
    public:
        MandarinG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class MandarinG2pInitArgs : public LangCore::G2pInitArgs {
    public:
        MandarinG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<MandarinG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::MandarinG2p::L1

#endif
