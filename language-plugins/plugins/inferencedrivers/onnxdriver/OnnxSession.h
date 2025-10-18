#ifndef DSINFER_ONNXSESSION_H
#define DSINFER_ONNXSESSION_H

#include <LangPlugins/Inference/InferenceSession.h>

namespace LangPlugins {

    class OnnxSession : public InferenceSession {
    public:
        OnnxSession();
        ~OnnxSession();

    public:
        LangMgr::Expected<void> open(const std::filesystem::path &path,
                                 const LangMgr::NO<InferenceSessionOpenArgs> &args) override;
        LangMgr::Expected<void> close() override;
        bool isOpen() const override;

        int64_t id() const override;

    public:
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>> start(const LangMgr::NO<LangMgr::TaskStartInput> &input) override;
        LangMgr::Expected<void> startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                       const StartAsyncCallback &callback) override;
        LangMgr::NO<LangMgr::TaskResult> result() const override;
        bool stop() override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        friend class OnnxTask;
    };

}

#endif // DSINFER_ONNXSESSION_H