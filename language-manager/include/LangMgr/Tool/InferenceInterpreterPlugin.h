#ifndef LANGMGR_INFERENCEINTERPRETERPLUGIN_H
#define LANGMGR_INFERENCEINTERPRETERPLUGIN_H

#include <LangMgr/Plugin/Plugin.h>
#include <LangMgr/Tool/InferenceInterpreter.h>

namespace LangMgr
{

    class InferenceInterpreterPlugin : public Plugin {
    public:
        InferenceInterpreterPlugin() = default;
        ~InferenceInterpreterPlugin() override;

        const char *iid() const override { return "org.openvpi.InferenceInterpreter"; }

        virtual NO<InferenceInterpreter> create() = 0;

        STDCORELIB_DISABLE_COPY(InferenceInterpreterPlugin)
    };
    inline InferenceInterpreterPlugin::~InferenceInterpreterPlugin() = default;

} // namespace LangMgr

#endif // LANGMGR_INFERENCEINTERPRETERPLUGIN_H
