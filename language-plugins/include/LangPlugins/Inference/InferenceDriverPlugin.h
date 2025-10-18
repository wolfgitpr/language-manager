#ifndef DSINFER_INFERENCEDRIVERPLUGIN_H
#define DSINFER_INFERENCEDRIVERPLUGIN_H

#include <LangMgr/Plugin/Plugin.h>
#include <LangPlugins/Inference/InferenceDriver.h>

namespace LangPlugins {

    class InferenceDriverPlugin : public LangMgr::Plugin {
    public:
        InferenceDriverPlugin() = default;
        ~InferenceDriverPlugin() = default;

        const char *iid() const override {
            return "org.openvpi.InferenceDriver";
        }

    public:
        virtual LangMgr::NO<InferenceDriver> create() = 0;

    public:
        STDCORELIB_DISABLE_COPY(InferenceDriverPlugin)
    };

}

#endif // DSINFER_INFERENCEDRIVERPLUGIN_H