#ifndef LANGPLUGINS_ONNXSESSION_H
#define LANGPLUGINS_ONNXSESSION_H

#include <LangCore/Task/Task.h>

namespace LangPlugins::OnnxDriver::V1
{

    class OnnxSession : public LangCore::SessionTask {
    public:
        OnnxSession();
        ~OnnxSession() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<void> open(const std::filesystem::path &path,
                                      const LangCore::NO<LangCore::TaskInitArgs> &args) override;
        LangCore::Expected<void> close() override;
        bool isOpen() const override;

        int64_t id() const override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        friend class OnnxTask;
    };

} // namespace LangPlugins::OnnxDriver::V1

#endif // LANGPLUGINS_ONNXSESSION_H
