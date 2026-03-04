#ifndef LANGCORE_TAGGERTASK_H
#define LANGCORE_TAGGERTASK_H

#include <filesystem>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Base/NamedObject.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    constexpr char TAGGER_API_NAME[] = "Tagger";
    constexpr char TAGGER_API_CLASS[] = "tagger.common.TaggerTask";
    constexpr int TAGGER_API_LEVEL = 1;

    class TaggerOutput : public TaskResult {
    public:
        TaggerOutput(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<TaggerRes> taggerResult;
        std::string errorMessage;
    };

    class TaggerConfiguration : public TaskConfiguration {
    public:
        TaggerConfiguration(std::string name, std::string iid, const int apiLevel) :
            TaskConfiguration(std::move(name), std::move(iid), apiLevel) {}

        std::string regexStr;
    };

    class TaggerRuntimeOptions : public TaskRuntimeOptions {
    public:
        TaggerRuntimeOptions(std::string name, std::string iid, const int apiLevel) :
            TaskRuntimeOptions(std::move(name), std::move(iid), apiLevel) {}
    };

    class TaggerInitArgs : public TaskInitArgs {
    public:
        TaggerInitArgs(std::string name, std::string iid, const int apiLevel) :
            TaskInitArgs(std::move(name), std::move(iid), apiLevel) {}

        NO<TaggerRuntimeOptions> runtimeOptions;
    };

    class TaggerStartInput : public TaskStartInput {
    public:
        TaggerStartInput(std::string name, std::string iid, const int apiLevel) :
            TaskStartInput(std::move(name), std::move(iid), apiLevel) {}

        std::vector<TaggerRes> taggerInput;
    };

    class TaggerResult : public TaskResult {
    public:
        TaggerResult(std::string name, std::string iid, const int apiLevel) :
            TaskResult(std::move(name), std::move(iid), apiLevel) {}

        std::vector<TaggerRes> taggerResult;
        std::string errorMessage;
    };
} // namespace LangCore
#endif // LANGCORE_TAGGERTASK_H
