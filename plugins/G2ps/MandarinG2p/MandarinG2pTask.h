#ifndef LANGPLUGINS_MANDARING2PTASK_H
#define LANGPLUGINS_MANDARING2PTASK_H

#include <LangCore/Task/Task.h>

namespace LangPlugins::MandarinG2p::V1
{
    class MandarinG2pTask : public LangCore::Task {
    public:
        explicit MandarinG2pTask(const LangCore::ModuleSpec *spec);
        ~MandarinG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        LangCore::Expected<void> updateConfig(const std::string &config);

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::MandarinG2p::V1

#endif // LANGPLUGINS_MANDARING2PTASK_H
