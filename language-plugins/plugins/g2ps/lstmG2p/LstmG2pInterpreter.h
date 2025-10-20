#ifndef LANGPLUGINS_DURATIONINTERPRETER_H
#define LANGPLUGINS_DURATIONINTERPRETER_H

#include <LangMgr/Tool/InferenceInterpreter.h>

namespace LangPlugins
{
    class LstmG2pInterpreter : public LangMgr::InferenceInterpreter {
    public:
        LstmG2pInterpreter();
        ~LstmG2pInterpreter() override;

        int apiLevel() const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::InferenceSchema>>
        createSchema(const LangMgr::InferenceSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::InferenceConfiguration>>
        createConfiguration(const LangMgr::InferenceSpec *spec) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::InferenceImportOptions>>
        createImportOptions(const LangMgr::InferenceSpec *spec, const LangMgr::JsonValue &options) const override;
        LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
        createInference(const LangMgr::InferenceSpec *spec,
                        const LangMgr::NO<LangMgr::InferenceImportOptions> &importOptions,
                        const LangMgr::NO<LangMgr::InferenceRuntimeOptions> &runtimeOptions) override;
    };

} // namespace LangPlugins

#endif // LANGPLUGINS_DURATIONINTERPRETER_H
