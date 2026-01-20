#ifndef LANGPLUGINS_LSTMG2PENGINEFACTORY_H
#define LANGPLUGINS_LSTMG2PENGINEFACTORY_H

#include <LangMgr/Task/TaskFactory.h>

namespace LangPlugins
{
    class LstmG2pEngineFactory : public LangMgr::TaskFactory {
    public:
        LstmG2pEngineFactory();
        ~LstmG2pEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleSpec *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_LSTMG2PENGINEFACTORY_H
