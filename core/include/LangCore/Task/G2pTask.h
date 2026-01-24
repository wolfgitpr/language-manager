#ifndef LANGCORE_G2PTASK_H
#define LANGCORE_G2PTASK_H

#include <filesystem>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Base/NamedObject.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    constexpr char G2P_API_NAME[] = "G2p";
    constexpr char G2P_API_CLASS[] = "g2p.common.G2pInference";
    constexpr int G2P_API_LEVEL = 1;

    class G2pOutput : public TaskResult {
    public:
        G2pOutput(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<G2pRes> g2pResult;
        std::string errorMessage;
    };

    class G2pConfiguration : public TaskConfiguration {
    public:
        G2pConfiguration(std::string name, std::string iid, const int apiLevel) :
            TaskConfiguration(std::move(name), std::move(iid), apiLevel) {}

        std::string regexStr;
    };

    class G2pRuntimeOptions : public TaskRuntimeOptions {
    public:
        G2pRuntimeOptions(std::string name, std::string iid, const int apiLevel) :
            TaskRuntimeOptions(std::move(name), std::move(iid), apiLevel) {}
    };

    class G2pInitArgs : public TaskInitArgs {
    public:
        G2pInitArgs(std::string name, std::string iid, const int apiLevel) :
            TaskInitArgs(std::move(name), std::move(iid), apiLevel) {}

        NO<G2pRuntimeOptions> runtimeOptions;
    };

    class G2pStartInput : public TaskStartInput {
    public:
        G2pStartInput(std::string name, std::string iid, const int apiLevel) :
            TaskStartInput(std::move(name), std::move(iid), apiLevel) {}

        std::vector<std::string> g2pInput;
    };

    class G2pResult : public TaskResult {
    public:
        G2pResult(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<G2pRes> g2pResult;
        std::string errorMessage;
    };
} // namespace LangCore
#endif // LANGCORE_G2PTASK_H
