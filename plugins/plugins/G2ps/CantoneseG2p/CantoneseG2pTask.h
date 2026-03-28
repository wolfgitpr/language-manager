#ifndef LANGPLUGINS_CANTONESEG2PTASK_H
#define LANGPLUGINS_CANTONESEG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

namespace LangPlugins::CantoneseG2p
{

    class CantoneseG2pTask : public LangCore::Task {
    public:
        explicit CantoneseG2pTask(const LangCore::ModuleSpec *spec);
        ~CantoneseG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace LangPlugins::CantoneseG2p

#endif
