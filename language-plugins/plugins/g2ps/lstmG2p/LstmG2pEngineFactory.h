#ifndef LANGPLUGINS_LSTMG2PENGINEFACTORY_H
#define LANGPLUGINS_LSTMG2PENGINEFACTORY_H

#include <LangMgr/Modules/EngineFactory.h>

namespace LangPlugins
{
    class LstmG2pEngineFactory : public LangMgr::EngineFactory {
    public:
        LstmG2pEngineFactory();
        ~LstmG2pEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleDefinition *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleDefinition *definition,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_LSTMG2PENGINEFACTORY_H
