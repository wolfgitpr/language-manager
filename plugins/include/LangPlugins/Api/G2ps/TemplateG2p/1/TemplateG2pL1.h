#ifndef LANGPLUGINS_API_TEMPLATEG2PAPIL1_H
#define LANGPLUGINS_API_TEMPLATEG2PAPIL1_H

#include <string>
#include <vector>

#include <LangCore/Task/G2pTask.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::Api::TemplateG2p::L1
{

    constexpr char API_NAME[] = "templateG2p";
    constexpr char API_CLASS[] = "g2p.template.TemplateG2pInference";
    constexpr int API_LEVEL = 1;

    class TemplateG2pConfiguration : public LangCore::G2pConfiguration {
    public:
        TemplateG2pConfiguration() : G2pConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::vector<InferUtil::VerifyEntry> verifyEntry;
        bool enableDict;
        std::filesystem::path dictPath;

        bool enableOnnxG2p;
        std::string onnxG2pId;
    };

    class TemplateG2pRuntimeOptions : public LangCore::G2pRuntimeOptions {
    public:
        TemplateG2pRuntimeOptions() : G2pRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class TemplateG2pInitArgs : public LangCore::G2pInitArgs {
    public:
        TemplateG2pInitArgs() : G2pInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        LangCore::NO<TemplateG2pRuntimeOptions> runtimeOptions;
    };

} // namespace LangPlugins::Api::TemplateG2p::L1

#endif // LANGPLUGINS_API_TEMPLATEG2PAPIL1_H
