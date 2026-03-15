#ifndef LANGPLUGINS_CHINESEG2PENGINEFACTORY_H
#define LANGPLUGINS_CHINESEG2PENGINEFACTORY_H

#include <LangCore/Task/TaskFactory.h>

namespace LangPlugins::ChineseG2p
{
    class ChineseG2pEngineFactory : public LangCore::TaskFactory {
    public:
        ChineseG2pEngineFactory();
        ~ChineseG2pEngineFactory() override;

        int apiLevel() const override;
        LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
        createConfiguration(const LangCore::ModuleSpec *spec) const override;
        LangCore::Expected<LangCore::NO<LangCore::Task>>
        createTask(const LangCore::ModuleSpec *spec,
                   const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_CHINESEG2PENGINEFACTORY_H
