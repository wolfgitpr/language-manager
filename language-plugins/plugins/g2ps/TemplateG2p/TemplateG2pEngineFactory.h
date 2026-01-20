#ifndef LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H
#define LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H

#include <LangMgr/Task/TaskFactory.h>

namespace LangPlugins
{
    class TemplateG2pEngineFactory : public LangMgr::TaskFactory {
    public:
        TemplateG2pEngineFactory();
        ~TemplateG2pEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleSpec *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_TemplateG2pENGINEFACTORY_H
