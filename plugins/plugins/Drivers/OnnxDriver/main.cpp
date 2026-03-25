#include <LangCore/Task/TaskFactoryPlugin.h>

#include "OnnxDriver.h"

namespace LangPlugins
{

    class OnnxDriverPlugin : public LangCore::DriverFactoryPlugin {
    public:
        OnnxDriverPlugin() = default;

        const char *key() const override { return "onnx"; }

        LangCore::NO<LangCore::SessionFactory> create() override { return LangCore::NO<OnnxDriver>::create(); }
    };

} // namespace LangPlugins

LANGCORE_EXPORT_PLUGIN(LangPlugins::OnnxDriverPlugin)
