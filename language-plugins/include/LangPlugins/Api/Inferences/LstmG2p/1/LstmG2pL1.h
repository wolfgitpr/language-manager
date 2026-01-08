#ifndef LANGMGR_API_LSTMG2PAPIL1_H
#define LANGMGR_API_LSTMG2PAPIL1_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace LangPlugins::Api::LstmG2p::L1
{

    constexpr char API_NAME[] = "lstmG2p";
    constexpr char API_CLASS[] = "g2p.model.LstmG2pInference";
    constexpr int API_LEVEL = 1;


    class LstmG2pConfiguration : public LangMgr::TaskConfiguration {
    public:
        LstmG2pConfiguration() : TaskConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

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

    class LstmG2pRuntimeOptions : public LangMgr::TaskRuntimeOptions {
    public:
        LstmG2pRuntimeOptions() : TaskRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}

        std::string device = "cpu";
        bool optimizePerformance = false;
    };

    class LstmG2pInitArgs : public LangMgr::TaskInitArgs {
    public:
        LstmG2pInitArgs() : TaskInitArgs(API_NAME) {}

        LangMgr::NO<LstmG2pRuntimeOptions> runtimeOptions;
    };

    struct G2pWord {
        std::string text;
        std::string language;
        std::vector<std::string> expectedPhonemes;
    };

    class LstmG2pStartInput : public LangMgr::TaskStartInput {
    public:
        LstmG2pStartInput() : TaskStartInput(API_NAME) {}

        std::vector<G2pWord> words;
        bool returnDetailedInfo = false;
    };

    class LstmG2pResult : public LangMgr::TaskResult {
    public:
        LstmG2pResult() : TaskResult(API_NAME) {}

        std::string word;
        std::vector<std::string> phonemes;

        std::string errorMessage;
    };

} // namespace LangPlugins::Api::LstmG2p::L1

#endif // LANGMGR_API_LSTMG2PAPIL1_H
