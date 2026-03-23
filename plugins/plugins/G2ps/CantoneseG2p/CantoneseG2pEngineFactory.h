#ifndef LANGPLUGINS_CANTONESEG2PENGINEFACTORY_H
#define LANGPLUGINS_CANTONESEG2PENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins::CantoneseG2p
{
    class CantoneseG2pEngineFactory : public LangCore::TaskFactory {
    public:
        CantoneseG2pEngineFactory();
        ~CantoneseG2pEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins::CantoneseG2p

#endif
