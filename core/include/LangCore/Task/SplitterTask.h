#ifndef LANGCORE_SPLITTERTASK_H
#define LANGCORE_SPLITTERTASK_H

#include <filesystem>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    constexpr char SPLITTER_API_NAME[] = "Splitter";
    constexpr char SPLITTER_API_CLASS[] = "splitter.common.RegexSplitterInference";
    constexpr int SPLITTER_API_LEVEL = 1;

    class SplitterOutput : public TaskResult {
    public:
        SplitterOutput(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<std::string> splitterResult;
        std::string errorMessage;
    };

    class SplitterConfiguration : public TaskConfiguration {
    public:
        SplitterConfiguration(std::string name, std::string iid, const int apiLevel) :
            TaskConfiguration(std::move(name), std::move(iid), apiLevel) {}

        std::string regexStr;
    };

    class SplitterRuntimeOptions : public TaskRuntimeOptions {
    public:
        SplitterRuntimeOptions(std::string name, std::string iid, const int apiLevel) :
            TaskRuntimeOptions(std::move(name), std::move(iid), apiLevel) {}
    };

    class SplitterInitArgs : public TaskInitArgs {
    public:
        SplitterInitArgs(std::string name, std::string iid, const int apiLevel) :
            TaskInitArgs(std::move(name), std::move(iid), apiLevel) {}

        NO<SplitterRuntimeOptions> runtimeOptions;
    };

    class SplitterStartInput : public TaskStartInput {
    public:
        SplitterStartInput(std::string name, std::string iid, const int apiLevel) :
            TaskStartInput(std::move(name), std::move(iid), apiLevel) {}

        std::vector<std::string> splitterInput;
    };

    class SplitterResult : public TaskResult {
    public:
        SplitterResult(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<std::string> splitterResult;
    };
} // namespace LangCore
#endif // LANGCORE_SPLITTERTASK_H
