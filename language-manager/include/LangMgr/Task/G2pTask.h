#ifndef LANGMGR_G2PTASK_H
#define LANGMGR_G2PTASK_H

#include <filesystem>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Task/Task.h>

namespace LangMgr
{
    constexpr char API_NAME[] = "G2p";
    constexpr char API_CLASS[] = "g2p.common.G2pInference";
    constexpr int API_LEVEL = 1;

    class G2pOutput : public TaskResult {
    public:
        G2pOutput() : TaskResult("G2pOutput") {}

        std::vector<G2pRes> g2pResult;
        std::string errorMessage;
    };

    class G2pConfiguration : public TaskConfiguration {
    public:
        G2pConfiguration() : TaskConfiguration(API_NAME, API_CLASS, API_LEVEL) {}

        std::string regexStr;
    };

    class G2pRuntimeOptions : public TaskRuntimeOptions {
    public:
        G2pRuntimeOptions() : TaskRuntimeOptions(API_NAME, API_CLASS, API_LEVEL) {}
    };

    class G2pInitArgs : public TaskInitArgs {
    public:
        G2pInitArgs() : TaskInitArgs(API_NAME) {}

        NO<G2pRuntimeOptions> runtimeOptions;
    };

    class G2pStartInput : public TaskStartInput {
    public:
        G2pStartInput() : TaskStartInput(API_NAME) {}

        std::vector<G2pInput> g2pInput;
    };

    class G2pResult : public TaskResult {
    public:
        G2pResult() : TaskResult(API_NAME) {}

        std::vector<G2pRes> g2pResult;
        std::string errorMessage;
    };
} // namespace LangMgr
#endif // LANGMGR_G2PTASK_H
