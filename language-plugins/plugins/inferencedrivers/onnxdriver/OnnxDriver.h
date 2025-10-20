#ifndef DSINFER_ONNXDRIVER_H
#define DSINFER_ONNXDRIVER_H

#include <filesystem>

#include <LangPlugins/Inference/InferenceDriver.h>

namespace LangPlugins
{

    class OnnxDriver : public InferenceDriver {
    public:
        OnnxDriver();
        ~OnnxDriver() override;

    public:
        std::string arch() const override;
        std::string backend() const override;

        LangMgr::Expected<void> initialize(const LangMgr::NO<InferenceDriverInitArgs> &args) override;
        LangMgr::NO<InferenceSession> createSession() override;
        LangMgr::Expected<void> loadFromProcess() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins

#endif // DSINFER_ONNXDRIVER_H
