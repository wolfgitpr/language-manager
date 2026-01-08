#ifndef LANGPLUGINS_REGEXSPLITTERENGINEFACTORY_H
#define LANGPLUGINS_REGEXSPLITTERENGINEFACTORY_H

#include <LangMgr/Modules/EngineFactory.h>

namespace LangPlugins
{
    class RegexSplitterEngineFactory : public LangMgr::EngineFactory {
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
