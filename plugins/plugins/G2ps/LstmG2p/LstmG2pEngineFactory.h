#ifndef LANGPLUGINS_LSTMG2PENGINEFACTORY_H
#define LANGPLUGINS_LSTMG2PENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins
{
    class LstmG2pEngineFactory : public LangCore::TaskFactory {
    public:
        LstmG2pEngineFactory();
        ~LstmG2pEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_LSTMG2PENGINEFACTORY_H
