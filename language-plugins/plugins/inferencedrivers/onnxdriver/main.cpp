#include <LangPlugins/Inference/InferenceDriverPlugin.h>

#include "OnnxDriver.h"

namespace LangPlugins
{

    class OnnxDriverPlugin : public InferenceDriverPlugin {
    public:
        OnnxDriverPlugin() = default;

    public:
        const char *key() const override { return "onnx"; }

        LangMgr::NO<InferenceDriver> create() override { return LangMgr::NO<OnnxDriver>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::OnnxDriverPlugin)
