#ifndef LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
#define LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H

#include <LangMgr/Task/TaskFactory.h>

namespace LangPlugins
{
    class RegexTaggerEngineFactory : public LangMgr::TaskFactory {
    public:
        RegexTaggerEngineFactory();
        ~RegexTaggerEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleSpec *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_REGEXTAGGERENGINEFACTORY_H
