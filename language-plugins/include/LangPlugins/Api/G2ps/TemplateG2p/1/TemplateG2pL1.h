#ifndef LANGMGR_API_TEMPLATEG2PAPIL1_H
#define LANGMGR_API_TEMPLATEG2PAPIL1_H

#include <string>
#include <vector>

#include <LangMgr/Task/Task.h>
#include <LangPlugins/Api/Common/1/CommonApiL1.h>

namespace LangPlugins::Api::TemplateG2p::L1
{

    struct VerifyEntry {
        std::string type;
        std::vector<std::string> value;
        std::string mode;
    };

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.TemplateInference";
    constexpr int API_LEVEL = 1;


    class TemplateG2pConfiguration : public LangMgr::TaskConfiguration {
    public:
        TemplateG2pConfiguration() : TaskConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<VerifyEntry> verifyEntry;
        std::filesystem::path dictPath;
        std::string onnxInferenceId;
    };

    class TemplateG2pRuntimeOptions : public LangMgr::TaskRuntimeOptions {
    public:
        TemplateG2pRuntimeOptions() : TaskRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class TemplateG2pInitArgs : public LangMgr::TaskInitArgs {
    public:
        TemplateG2pInitArgs() : TaskInitArgs(API_NAME) {}

        LangMgr::NO<TemplateG2pRuntimeOptions> runtimeOptions;
    };


    class TemplateG2pStartInput : public LangMgr::TaskStartInput {
    public:
        TemplateG2pStartInput() : TaskStartInput(API_NAME) {}

        std::vector<Common::L1::G2pInput> g2pInput;
    };

    class TemplateG2pResult : public LangMgr::TaskResult {
    public:
        TemplateG2pResult() : TaskResult(API_NAME) {}

        std::vector<Common::L1::G2pRes> g2pResult;
        std::string errorMessage;
    };

} // namespace LangPlugins::Api::TemplateG2p::L1

#endif // LANGMGR_API_TEMPLATEG2PAPIL1_H
