#ifndef LANGPLUGINS_ONNXSESSION_H
#define LANGPLUGINS_ONNXSESSION_H

#include <LangCore/Task/Task.h>

namespace LangPlugins
{

    class OnnxSession : public LangCore::SessionTask {
    public:
        OnnxSession();
        ~OnnxSession() override;

        LangCore::Expected<void> open(const std::filesystem::path &path,
                                      const LangCore::NO<LangCore::TaskInitArgs> &args) override;
        LangCore::Expected<void> close() override;
        bool isOpen() const override;

        int64_t id() const override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;
        LangCore::Expected<void> startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                            const StartAsyncCallback &callback) override;
        LangCore::NO<LangCore::TaskResult> result() const override;
        bool stop() override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        friend class OnnxTask;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_ONNXSESSION_H
