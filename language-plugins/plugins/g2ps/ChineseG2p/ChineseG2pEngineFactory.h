#ifndef LANGPLUGINS_CHINESEG2PENGINEFACTORY_H
#define LANGPLUGINS_CHINESEG2PENGINEFACTORY_H

#include <LangMgr/Task/TaskFactory.h>

namespace LangPlugins
{
    class ChineseG2pEngineFactory : public LangMgr::TaskFactory {
    public:
        ChineseG2pEngineFactory();
        ~ChineseG2pEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleSpec *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_CHINESEG2PENGINEFACTORY_H
