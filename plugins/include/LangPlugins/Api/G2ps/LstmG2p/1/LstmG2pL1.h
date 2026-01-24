#ifndef LANGPLUGINS_API_LSTMG2PAPIL1_H
#define LANGPLUGINS_API_LSTMG2PAPIL1_H

#include <filesystem>
#include <map>
#include <string>

#include <LangCore/Task/G2pTask.h>

namespace LangPlugins::Api::LstmG2p::L1
{

    constexpr char API_NAME[] = "lstmG2p";
    constexpr char API_CLASS[] = "g2p.model.LstmG2pInference";
    constexpr int API_LEVEL = 1;

    class LstmG2pConfiguration : public LangCore::G2pConfiguration {
    public:
        LstmG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::map<std::string, int> charVocab;
        std::map<std::string, int> phonemeVocab;
        std::map<int, std::string> idx_to_phoneme;

        std::filesystem::path encoder;
        std::filesystem::path decoder;

        int unkIdx = 0;
        int padIdx = 1;
        int bosIdx = 2;
        int eosIdx = 3;
        int maxLen = 48;
    };

    class LstmG2pRuntimeOptions : public LangCore::G2pRuntimeOptions {
    public:
        LstmG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}

        std::string device = "cpu";
        bool optimizePerformance = false;
    };

    class LstmG2pInitArgs : public LangCore::G2pInitArgs {
    public:
        LstmG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<LstmG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::LstmG2p::L1

#endif // LANGPLUGINS_API_LSTMG2PAPIL1_H
