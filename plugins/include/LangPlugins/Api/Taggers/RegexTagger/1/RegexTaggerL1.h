#ifndef LANGMGR_REGEXTAGGERL1_H
#define LANGMGR_REGEXTAGGERL1_H

#include <string>
#include <vector>

#include <LangCore/Task/TaggerTask.h>

namespace LangPlugins::Api::RegexTagger::L1
{
    constexpr char API_NAME[] = "regexTagger";
    constexpr char API_CLASS[] = "tagger.regex.RegexTaggerInference";
    constexpr int API_LEVEL = 1;

    struct TaggerRegexEntry {
        std::vector<std::string> regexes;
        std::string tag;
    };

    class RegexTaggerConfiguration : public LangCore::TaggerConfiguration {
    public:
        RegexTaggerConfiguration() : TaggerConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string language;
        std::vector<TaggerRegexEntry> regexEntry;
    };

    class RegexTaggerRuntimeOptions : public LangCore::TaggerRuntimeOptions {
    public:
        RegexTaggerRuntimeOptions() : TaggerRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexTaggerInitArgs : public LangCore::TaggerInitArgs {
    public:
        RegexTaggerInitArgs() : TaggerInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<RegexTaggerRuntimeOptions> runtimeOptions;
    };

    class RegexTaggerStartInput : public LangCore::TaggerStartInput {
    public:
        RegexTaggerStartInput() : TaggerStartInput(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class RegexTaggerResult : public LangCore::TaggerResult {
    public:
        RegexTaggerResult() : TaggerResult(API_NAME, API_CLASS, API_LEVEL) {}
        std::string errorMessage;
    };

} // namespace LangPlugins::Api::RegexTagger::L1
#endif // LANGMGR_REGEXTAGGERL1_H
