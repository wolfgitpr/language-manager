#ifndef LANGMGR_REGEXSPLITTERL1_H
#define LANGMGR_REGEXSPLITTERL1_H

#include <string>
#include <vector>

#include <LangMgr/Task/SplitterTask.h>

namespace LangPlugins::Api::RegexSplitter::L1
{
    constexpr char API_NAME[] = "regexSplitter";
    constexpr char API_CLASS[] = "splitter.regex.RegexSplitterInference";
    constexpr int API_LEVEL = 1;


    class RegexSplitterConfiguration : public LangMgr::SplitterConfiguration {
    public:
        RegexSplitterConfiguration() : SplitterConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string regexStr;
    };

    class RegexSplitterRuntimeOptions : public LangMgr::SplitterRuntimeOptions {
    public:
        RegexSplitterRuntimeOptions() : SplitterRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexSplitterInitArgs : public LangMgr::SplitterInitArgs {
    public:
        RegexSplitterInitArgs() : SplitterInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangMgr::NO<RegexSplitterRuntimeOptions> runtimeOptions;
    };

    class RegexSplitterStartInput : public LangMgr::SplitterStartInput {
    public:
        RegexSplitterStartInput() : SplitterStartInput(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<std::string> rawStrVec;
    };

    class RegexSplitterResult : public LangMgr::SplitterResult {
    public:
        RegexSplitterResult() : SplitterResult(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<std::string> resStrVec;

        std::string errorMessage;
    };

} // namespace LangPlugins::Api::RegexSplitter::L1
#endif // LANGMGR_REGEXSPLITTERL1_H
