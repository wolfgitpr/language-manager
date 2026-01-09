#ifndef LANGMGR_REGEXSPLITTERL1_H
#define LANGMGR_REGEXSPLITTERL1_H

#include <string>
#include <vector>

#include <LangMgr/Module/G2pModule.h>


namespace LangPlugins::Api::RegexSplitter::L1
{

    constexpr char API_NAME[] = "regexSplitter";
    constexpr char API_CLASS[] = "splitter.regex.RegexSplitterInference";
    constexpr int API_LEVEL = 1;


    class RegexSplitterConfiguration : public LangMgr::TaskConfiguration {
    public:
        RegexSplitterConfiguration() : TaskConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string regexStr;
    };

    class RegexSplitterRuntimeOptions : public LangMgr::TaskRuntimeOptions {
    public:
        RegexSplitterRuntimeOptions() : TaskRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexSplitterInitArgs : public LangMgr::TaskInitArgs {
    public:
        RegexSplitterInitArgs() : TaskInitArgs(API_NAME) {}

        LangMgr::NO<RegexSplitterRuntimeOptions> runtimeOptions;
    };

    class RegexSplitterStartInput : public LangMgr::TaskStartInput {
    public:
        RegexSplitterStartInput() : TaskStartInput(API_NAME) {}

        std::vector<std::string> rawStrVec;
    };

    class RegexSplitterResult : public LangMgr::TaskResult {
    public:
        RegexSplitterResult() : TaskResult(API_NAME) {}

        std::vector<std::string> resStrVec;

        std::string errorMessage;
    };

} // namespace LangPlugins::Api::RegexSplitter::L1
#endif // LANGMGR_REGEXSPLITTERL1_H
