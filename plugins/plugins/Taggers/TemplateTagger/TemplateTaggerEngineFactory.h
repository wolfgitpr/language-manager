#ifndef LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
#define LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins::TemplateTagger
{
    class TemplateTaggerEngineFactory : public LangCore::TaskFactory {
    public:
        TemplateTaggerEngineFactory();
        ~TemplateTaggerEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins::TemplateTagger

#endif // LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
