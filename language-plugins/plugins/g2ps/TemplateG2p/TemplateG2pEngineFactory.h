#ifndef LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H
#define LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H

#include <LangMgr/Modules/EngineFactory.h>

namespace LangPlugins
{
    class TemplateG2pEngineFactory : public LangMgr::EngineFactory {
    public:
        TemplateG2pEngineFactory();
        ~TemplateG2pEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleDefinition *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleDefinition *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_TemplateG2pENGINEFACTORY_H
