#ifndef DSINFER_ONNXDRIVER_SESSION_H
#define DSINFER_ONNXDRIVER_SESSION_H

#include <map>
#include <memory>
#include <filesystem>
#include <functional>

#include <LangMgr/Support/Expected.h>
#include <LangPlugins/Api/Drivers/Onnx/OnnxDriverApi.h>
#include <LangMgr/Task/ITask.h>


namespace LangPlugins::onnxdriver {

    class Session {
    public:
        enum SessionHint {
            SH_NoHint,
            SH_PreferCPUHint = 0x1,
        };

        Session();
        ~Session();

        Session(const Session &) = delete;
        Session &operator=(const Session &) = delete;

        Session(Session &&other) noexcept;
        Session &operator=(Session &&other) noexcept;

    public:
        LangMgr::Expected<void> open(const std::filesystem::path &path, const LangMgr::NO<Api::Onnx::SessionOpenArgs> &args);
        LangMgr::Expected<void> close();

        const std::vector<std::string> &inputNames() const;
        const std::vector<std::string> &outputNames() const;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>> run(const LangMgr::NO<LangMgr::TaskStartInput> &input);
        LangMgr::Expected<void> runAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input, const LangMgr::ITask::StartAsyncCallback &callback);

        void terminate();

        const std::filesystem::path &path() const;
        bool isOpen() const;

        LangMgr::NO<LangMgr::TaskResult> result() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}

#endif // DSINFER_ONNXDRIVER_SESSION_H
