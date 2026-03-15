#ifndef LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
#define LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins::RegexSplitter
{
    class RegexSplitterEngineFactory : public LangCore::TaskFactory {
    public:
        RegexSplitterEngineFactory();
        ~RegexSplitterEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins::RegexSplitter

#endif // LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
