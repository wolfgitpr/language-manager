#ifndef LANGPLUGINS_ONNXDRIVER_H
#define LANGPLUGINS_ONNXDRIVER_H

#include <filesystem>

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins
{

    class OnnxDriver : public LangCore::SessionFactory {
    public:
        OnnxDriver();
        ~OnnxDriver() override;

        std::string arch() const override;
        std::string backend() const override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;
        LangCore::NO<LangCore::SessionTask> createSession() override;
        LangCore::Expected<void> loadFromProcess() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_ONNXDRIVER_H
