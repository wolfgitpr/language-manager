#ifndef LANGMGR_ONNXDRIVER_SESSION_H
#define LANGMGR_ONNXDRIVER_SESSION_H

#include <filesystem>
#include <memory>

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Task/Task.h>
#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>


namespace LangPlugins::onnxDriver
{

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

        LangMgr::Expected<void> open(const std::filesystem::path &path,
                                     const LangMgr::NO<Api::Onnx::L1::SessionOpenArgs> &args);
        LangMgr::Expected<void> close();

        const std::vector<std::string> &inputNames() const;
        const std::vector<std::string> &outputNames() const;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>> run(const LangMgr::NO<LangMgr::TaskStartInput> &input);
        LangMgr::Expected<void> runAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                         const LangMgr::Task::StartAsyncCallback &callback);

        void terminate();

        const std::filesystem::path &path() const;
        bool isOpen() const;

        LangMgr::NO<LangMgr::TaskResult> result() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::onnxDriver

#endif // LANGMGR_ONNXDRIVER_SESSION_H
