#ifndef LANGPLUGINS_ONNXDRIVER_SESSION_H
#define LANGPLUGINS_ONNXDRIVER_SESSION_H

#include <filesystem>
#include <memory>

#include <LangCore/Support/Expected.h>
#include <LangCore/Task/Task.h>

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

        LangCore::Expected<void> open(const std::filesystem::path &path,
                                      const LangCore::NO<LangPlugins::Api::Onnx::L1::SessionOpenArgs> &args);
        LangCore::Expected<void> close();

        const std::vector<std::string> &inputNames() const;
        const std::vector<std::string> &outputNames() const;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>> run(const LangCore::NO<LangCore::TaskStartInput> &input);
        LangCore::Expected<void> runAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                          const LangCore::Task::StartAsyncCallback &callback);

        void terminate();

        const std::filesystem::path &path() const;
        bool isOpen() const;

        LangCore::NO<LangCore::TaskResult> result() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::onnxDriver

#endif // LANGPLUGINS_ONNXDRIVER_SESSION_H
