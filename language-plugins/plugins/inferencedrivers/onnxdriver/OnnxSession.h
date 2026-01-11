#ifndef LANGMGR_ONNXSESSION_H
#define LANGMGR_ONNXSESSION_H

#include <LangMgr/Task/Task.h>

namespace LangPlugins
{

    class OnnxSession : public LangMgr::SessionTask {
    public:
        OnnxSession();
        ~OnnxSession() override;

        LangMgr::Expected<void> open(const std::filesystem::path &path,
                                     const LangMgr::NO<LangMgr::TaskInitArgs> &args) override;
        LangMgr::Expected<void> close() override;
        bool isOpen() const override;

        int64_t id() const override;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
        start(const LangMgr::NO<LangMgr::TaskStartInput> &input) override;
        LangMgr::Expected<void> startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                           const StartAsyncCallback &callback) override;
        LangMgr::NO<LangMgr::TaskResult> result() const override;
        bool stop() override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        friend class OnnxTask;
    };

} // namespace LangPlugins

#endif // LANGMGR_ONNXSESSION_H
