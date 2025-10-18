#ifndef LANGMGR_INFERENCEINTERPRETERPLUGIN_H
#define LANGMGR_INFERENCEINTERPRETERPLUGIN_H

#include <LangMgr/Plugin/Plugin.h>
#include <LangMgr/Tool/InferenceInterpreter.h>

namespace LangMgr {

    class InferenceInterpreterPlugin : public Plugin {
    public:
        InferenceInterpreterPlugin() = default;
        ~InferenceInterpreterPlugin() = default;

        const char *iid() const override {
            return "org.openvpi.InferenceInterpreter";
        }

    public:
        virtual NO<InferenceInterpreter> create() = 0;

    public:
        STDCORELIB_DISABLE_COPY(InferenceInterpreterPlugin)
    };

}

#endif // LANGMGR_INFERENCEINTERPRETERPLUGIN_H