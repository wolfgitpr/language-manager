#ifndef LANGMGR_ONNXDRIVER_H
#define LANGMGR_ONNXDRIVER_H

#include <filesystem>

#include <LangMgr/Task/TaskFactoryPlugin.h>

namespace LangPlugins
{

    class OnnxDriver : public LangMgr::SessionFactory {
    public:
        OnnxDriver();
        ~OnnxDriver() override;

        std::string arch() const override;
        std::string backend() const override;

        LangMgr::Expected<void> initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) override;
        LangMgr::NO<LangMgr::SessionTask> createSession() override;
        LangMgr::Expected<void> loadFromProcess() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins

#endif // LANGMGR_ONNXDRIVER_H
