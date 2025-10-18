#ifndef DSINFER_INFERENCESESSION_H
#define DSINFER_INFERENCESESSION_H

#include <filesystem>

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Task/ITask.h>

#include <dsinfer/dsinfer_global.h>

namespace LangPlugins {

    class InferenceSessionOpenArgs : public LangMgr::NamedObject {
    public:
        inline InferenceSessionOpenArgs(std::string name, int version)
            : LangMgr::NamedObject(std::move(name)), version(version) {
        }

        int version;
    };

    class InferenceSessionInitArgs : public LangMgr::TaskInitArgs {
    public:
        inline InferenceSessionInitArgs(std::string name, int version)
            : LangMgr::TaskInitArgs(std::move(name)), version(version) {
        }

        int version;
    };


    class InferenceSessionStartInput : public LangMgr::TaskStartInput {
    public:
        inline InferenceSessionStartInput(std::string name, int version)
            : LangMgr::TaskStartInput(std::move(name)), version(version) {
        }

        int version;
    };

    class InferenceSessionResult : public LangMgr::TaskResult {
    public:
        inline InferenceSessionResult(std::string name, int version)
            : LangMgr::TaskResult(std::move(name)), version(version) {
        }

        int version;
    };

    /// InferenceSession - Provides a basic interface for the memory image of an AI model.
    class InferenceSession : public LangMgr::ITask {
    public:
        virtual LangMgr::Expected<void> open(const std::filesystem::path &path,
                                         const LangMgr::NO<InferenceSessionOpenArgs> &args) = 0;
        virtual LangMgr::Expected<void> close() = 0;
        virtual bool isOpen() const = 0;

        virtual int64_t id() const = 0;
    };

}

#endif // DSINFER_INFERENCESESSION_H