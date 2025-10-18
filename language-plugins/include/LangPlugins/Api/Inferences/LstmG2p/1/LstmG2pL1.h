#ifndef LANGMGR_API_LSTMG2PAPIL1_H
#define LANGMGR_API_LSTMG2PAPIL1_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <LangMgr/Tool/Inference.h>
#include <LangMgr/Tool/InferenceContrib.h>


namespace LangPlugins::Api::LstmG2p::L1
{

    inline constexpr char API_NAME[] = "lstmG2p";

    inline constexpr char API_CLASS[] = "ai.svs.LstmG2pInference";

    inline constexpr int API_LEVEL = 1;


    class LstmG2pSchema : public LangMgr::InferenceSchema {
    public:
        inline LstmG2pSchema() : LangMgr::InferenceSchema(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<std::string> languages;
    };

    class LstmG2pConfiguration : public LangMgr::InferenceConfiguration {
    public:
        inline LstmG2pConfiguration() : LangMgr::InferenceConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::map<std::string, int> charVocab;

        std::map<std::string, int> phonemeVocab;

        std::filesystem::path encoder;

        std::filesystem::path decoder;

        int unkIdx = 0;
        int padIdx = 1;
        int bosIdx = 2;
        int eosIdx = 3;
        int maxLen = 48;

        bool useBeamSearch = false;

        int beamSize = 5;
    };

    class LstmG2pImportOptions : public LangMgr::InferenceImportOptions {
    public:
        inline LstmG2pImportOptions() : LangMgr::InferenceImportOptions(API_NAME, API_CLASS, API_LEVEL) {}

        std::filesystem::path vocabPath;
        std::filesystem::path configPath;
    };

    class LstmG2pRuntimeOptions : public LangMgr::InferenceRuntimeOptions {
    public:
        inline LstmG2pRuntimeOptions() : LangMgr::InferenceRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}

        std::string device = "cpu";
        bool optimizePerformance = false;
    };

    class LstmG2pInitArgs : public LangMgr::InferenceInitArgs {
    public:
        inline LstmG2pInitArgs() : InferenceInitArgs(API_NAME) {}

        LangMgr::NO<LstmG2pRuntimeOptions> runtimeOptions;
    };

    struct G2pWord {
        std::string text;
        std::string language;
        std::vector<std::string> expectedPhonemes;
    };

    class LstmG2pStartInput : public LangMgr::TaskStartInput {
    public:
        inline LstmG2pStartInput() : LangMgr::TaskStartInput(API_NAME) {}

        std::vector<G2pWord> words;

        bool returnDetailedInfo = false;
    };

    class LstmG2pResult : public LangMgr::TaskResult {
    public:
        inline LstmG2pResult() : LangMgr::TaskResult(API_NAME) {}

        std::string word;
        std::vector<std::string> phonemes;

        std::string errorMessage;
    };

} // namespace LangPlugins::Api::LstmG2p::L1

#endif // LANGMGR_API_LSTMG2PAPIL1_H
