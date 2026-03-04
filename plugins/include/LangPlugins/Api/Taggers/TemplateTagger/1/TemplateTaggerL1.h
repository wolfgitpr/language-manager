#ifndef LANGMGR_REGEXTAGGERL1_H
#define LANGMGR_REGEXTAGGERL1_H

#include <string>
#include <vector>

#include <LangCore/Task/TaggerTask.h>

namespace LangPlugins::Api::TemplateTagger::L1
{
    constexpr char API_NAME[] = "templateTagger";
    constexpr char API_CLASS[] = "tagger.regex.TemplateTaggerInference";
    constexpr int API_LEVEL = 1;

    struct TaggerUtilEntry {
        std::string type;
        std::vector<std::string> value;
        std::string tag;
        bool discard = false;
    };

    class TemplateTaggerConfiguration : public LangCore::TaggerConfiguration {
    public:
        TemplateTaggerConfiguration() : TaggerConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string language;
        std::vector<TaggerUtilEntry> taggerUtilEntry;
    };

    class TemplateTaggerRuntimeOptions : public LangCore::TaggerRuntimeOptions {
    public:
        TemplateTaggerRuntimeOptions() : TaggerRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class TemplateTaggerInitArgs : public LangCore::TaggerInitArgs {
    public:
        TemplateTaggerInitArgs() : TaggerInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<TemplateTaggerRuntimeOptions> runtimeOptions;
    };

    class TemplateTaggerStartInput : public LangCore::TaggerStartInput {
    public:
        TemplateTaggerStartInput() : TaggerStartInput(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class TemplateTaggerResult : public LangCore::TaggerResult {
    public:
        TemplateTaggerResult() : TaggerResult(API_NAME, API_CLASS, API_LEVEL) {}
        std::string errorMessage;
    };

} // namespace LangPlugins::Api::TemplateTagger::L1
#endif // LANGMGR_REGEXTAGGERL1_H
