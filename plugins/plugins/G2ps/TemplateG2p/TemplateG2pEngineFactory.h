#ifndef LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H
#define LANGPLUGINS_TEMPLATEG2PENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins
{
    class TemplateG2pEngineFactory : public LangCore::TaskFactory {
    public:
        TemplateG2pEngineFactory();
        ~TemplateG2pEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_TemplateG2pENGINEFACTORY_H
