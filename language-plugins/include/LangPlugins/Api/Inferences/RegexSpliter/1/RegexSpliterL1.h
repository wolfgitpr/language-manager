#ifndef LANGMGR_REGEXSPLITERL1_H
#define LANGMGR_REGEXSPLITERL1_H

#include <string>
#include <vector>

#include <LangMgr/Tool/Inference.h>
#include <LangMgr/Tool/InferenceContrib.h>


namespace LangPlugins::Api::RegexSpliter::L1
{

    constexpr char API_NAME[] = "regexSpliter";
    constexpr char API_CLASS[] = "spliter.regex.RegexSpliterInference";
    constexpr int API_LEVEL = 1;


    class RegexSpliterConfiguration : public LangMgr::InferenceConfiguration {
    public:
        RegexSpliterConfiguration() : InferenceConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string regexStr;
    };

    class RegexSpliterRuntimeOptions : public LangMgr::InferenceRuntimeOptions {
    public:
        RegexSpliterRuntimeOptions() : InferenceRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexSpliterInitArgs : public LangMgr::InferenceInitArgs {
    public:
        RegexSpliterInitArgs() : InferenceInitArgs(API_NAME) {}

        LangMgr::NO<RegexSpliterRuntimeOptions> runtimeOptions;
    };

    class RegexSpliterStartInput : public LangMgr::TaskStartInput {
    public:
        RegexSpliterStartInput() : TaskStartInput(API_NAME) {}

        std::vector<std::string> rawStrVec;
    };

    class RegexSpliterResult : public LangMgr::TaskResult {
    public:
        RegexSpliterResult() : TaskResult(API_NAME) {}

        std::vector<std::string> resStrVec;

        std::string errorMessage;
    };

} // namespace LangPlugins::Api::RegexSpliter::L1
#endif // LANGMGR_REGEXSPLITERL1_H
