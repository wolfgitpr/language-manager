#ifndef LANGMGR_TEMPLATESPLITTERL1_H
#define LANGMGR_TEMPLATESPLITTERL1_H

#include <string>
#include <vector>

#include <LangCore/Task/SplitterTask.h>

namespace LangPlugins::Api::RegexSplitter::L1
{
    constexpr char API_NAME[] = "templateSplitter";
    constexpr char API_CLASS[] = "tagger.regex.RegexSplitterInference";
    constexpr int API_LEVEL = 1;

    class RegexSplitterConfiguration : public LangCore::SplitterConfiguration {
    public:
        RegexSplitterConfiguration() : SplitterConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<std::string> regexes;
    };

    class RegexSplitterRuntimeOptions : public LangCore::SplitterRuntimeOptions {
    public:
        RegexSplitterRuntimeOptions() : SplitterRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexSplitterInitArgs : public LangCore::SplitterInitArgs {
    public:
        RegexSplitterInitArgs() : SplitterInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<RegexSplitterRuntimeOptions> runtimeOptions;
    };

    class RegexSplitterStartInput : public LangCore::SplitterStartInput {
    public:
        RegexSplitterStartInput() : SplitterStartInput(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexSplitterResult : public LangCore::SplitterResult {
    public:
        RegexSplitterResult() : SplitterResult(API_NAME, API_CLASS, API_LEVEL) {}
        std::string errorMessage;
    };

} // namespace LangPlugins::Api::RegexSplitter::L1
#endif // LANGMGR_TEMPLATESPLITTERL1_H
