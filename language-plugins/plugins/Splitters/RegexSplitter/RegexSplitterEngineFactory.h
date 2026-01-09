#ifndef LANGPLUGINS_REGEXSPLITTERENGINEFACTORY_H
#define LANGPLUGINS_REGEXSPLITTERENGINEFACTORY_H

#include <../../../../language-manager/include/LangMgr/Task/TaskFactory.h>

namespace LangPlugins
{
    class RegexSplitterEngineFactory : public LangMgr::TaskFactory {
    public:
        RegexSplitterEngineFactory();
        ~RegexSplitterEngineFactory() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::TaskConfiguration>>
        createConfiguration(const LangMgr::ModuleDefinition *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
        createTask(const LangMgr::ModuleDefinition *spec,
                   const LangMgr::NO<LangMgr::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_REGEXSPLITTERENGINEFACTORY_H
