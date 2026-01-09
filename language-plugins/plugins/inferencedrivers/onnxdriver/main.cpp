#include <LangMgr/Task/TaskFactoryPlugin.h>

#include "OnnxDriver.h"

namespace LangPlugins
{

    class OnnxDriverPlugin : public LangMgr::DriverFactoryPlugin {
    public:
        OnnxDriverPlugin() = default;

    public:
        const char *key() const override { return "onnx"; }

        LangMgr::NO<LangMgr::SessionFactory> create() override { return LangMgr::NO<OnnxDriver>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::OnnxDriverPlugin)
