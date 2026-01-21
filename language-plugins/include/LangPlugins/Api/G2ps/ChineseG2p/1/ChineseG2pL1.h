#ifndef LANGMGR_API_CHINESEG2PAPIL1_H
#define LANGMGR_API_CHINESEG2PAPIL1_H

#include <vector>

#include <LangMgr/Task/G2pTask.h>

#include <inferutil/Verifier.h>

namespace LangPlugins::Api::ChineseG2p::L1
{

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.ChineseG2pInference";
    constexpr int API_LEVEL = 1;

    class ChineseG2pConfiguration : public LangMgr::G2pConfiguration {
    public:
        ChineseG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<inferUtil::VerifyEntry> verifyEntry;
        std::filesystem::path dictPath;
    };

    class ChineseG2pRuntimeOptions : public LangMgr::G2pRuntimeOptions {
    public:
        ChineseG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class ChineseG2pInitArgs : public LangMgr::G2pInitArgs {
    public:
        ChineseG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangMgr::NO<ChineseG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::ChineseG2p::L1

#endif // LANGMGR_API_CHINESEG2PAPIL1_H
