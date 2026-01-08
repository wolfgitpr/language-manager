#ifndef LANGPLUGINS_INFERENCESESSION_H
#define LANGPLUGINS_INFERENCESESSION_H

#include <filesystem>

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Task/Task.h>

namespace LangPlugins
{

    class InferenceSessionOpenArgs : public LangMgr::NamedObject {
    public:
        InferenceSessionOpenArgs(std::string name, const int version) :
            NamedObject(std::move(name)), version(version) {}

        int version;
    };

    class InferenceSessionInitArgs : public LangMgr::TaskInitArgs {
    public:
        InferenceSessionInitArgs(std::string name, const int version) :
            TaskInitArgs(std::move(name)), version(version) {}

        int version;
    };


    class InferenceSessionStartInput : public LangMgr::TaskStartInput {
    public:
        InferenceSessionStartInput(std::string name, const int version) :
            TaskStartInput(std::move(name)), version(version) {}

        int version;
    };

    class InferenceSessionResult : public LangMgr::TaskResult {
    public:
        InferenceSessionResult(std::string name, const int version) : TaskResult(std::move(name)), version(version) {}

        int version;
    };

    /// InferenceSession - Provides a basic interface for the memory image of an AI model.
    class InferenceSession : public LangMgr::Task {
    public:
        virtual LangMgr::Expected<void> open(const std::filesystem::path &path,
                                             const LangMgr::NO<InferenceSessionOpenArgs> &args) = 0;
        virtual LangMgr::Expected<void> close() = 0;
        virtual bool isOpen() const = 0;

        virtual int64_t id() const = 0;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_INFERENCESESSION_H
