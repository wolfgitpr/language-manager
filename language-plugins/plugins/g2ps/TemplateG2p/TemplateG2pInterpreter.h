#ifndef LANGPLUGINS_TEMPLATEG2PINTERPRETER_H
#define LANGPLUGINS_TEMPLATEG2PINTERPRETER_H

#include <LangMgr/Tool/InferenceInterpreter.h>

namespace LangPlugins
{
    class TemplateG2pInterpreter : public LangMgr::InferenceInterpreter {
    public:
        TemplateG2pInterpreter();
        ~TemplateG2pInterpreter() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::InferenceConfiguration>>
        createConfiguration(const LangMgr::InferenceSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
        createInference(const LangMgr::InferenceSpec *spec,
                        const LangMgr::NO<LangMgr::InferenceRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_TemplateG2pINTERPRETER_H
