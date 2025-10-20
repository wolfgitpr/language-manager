#ifndef DSINFER_INFERENCEDRIVERPLUGIN_H
#define DSINFER_INFERENCEDRIVERPLUGIN_H

#include <LangMgr/Plugin/Plugin.h>
#include <LangPlugins/Inference/InferenceDriver.h>

namespace LangPlugins
{

    class InferenceDriverPlugin : public LangMgr::Plugin {
    public:
        InferenceDriverPlugin() = default;
        ~InferenceDriverPlugin() override;

        const char *iid() const override { return "org.openvpi.InferenceDriver"; }

        virtual LangMgr::NO<InferenceDriver> create() = 0;

        STDCORELIB_DISABLE_COPY(InferenceDriverPlugin)
    };
    inline InferenceDriverPlugin::~InferenceDriverPlugin() {}

} // namespace LangPlugins

#endif // DSINFER_INFERENCEDRIVERPLUGIN_H
