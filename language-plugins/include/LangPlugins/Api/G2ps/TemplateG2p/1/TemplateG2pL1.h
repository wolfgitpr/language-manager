#ifndef LANGMGR_API_TEMPLATEG2PAPIL1_H
#define LANGMGR_API_TEMPLATEG2PAPIL1_H

#include <string>
#include <vector>

#include <LangMgr/Task/Task.h>

#include "LangMgr/Task/G2pTask.h"

namespace LangPlugins::Api::TemplateG2p::L1
{

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.TemplateG2pInference";
    constexpr int API_LEVEL = 1;

    struct VerifyEntry {
        std::string type;
        std::vector<std::string> value;
        std::string mode;
    };

    class TemplateG2pConfiguration : public LangMgr::G2pConfiguration {
    public:
        TemplateG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<VerifyEntry> verifyEntry;
        std::filesystem::path dictPath;
        std::string onnxG2pId;
    };

    class TemplateG2pRuntimeOptions : public LangMgr::G2pRuntimeOptions {
    public:
        TemplateG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class TemplateG2pInitArgs : public LangMgr::G2pInitArgs {
    public:
        TemplateG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangMgr::NO<TemplateG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::TemplateG2p::L1

#endif // LANGMGR_API_TEMPLATEG2PAPIL1_H
