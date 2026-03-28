#include <LangCore/Task/TaskPlugin.h>

#include "OnnxDriver.h"

namespace LangPlugins::OnnxDriver::V1
{

    class OnnxDriverPlugin : public LangCore::DriverPlugin {
    public:
        OnnxDriverPlugin() = default;

        int apiLevel() const override { return 1; }

        const char *key() const override { return "onnx"; }

        LangCore::Expected<LangCore::NO<LangCore::SessionFactory>> create() override {
            return LangCore::NO<OnnxDriver>::create();
        }
    };

} // namespace LangPlugins

LANGCORE_EXPORT_PLUGIN(LangPlugins::OnnxDriver::V1::OnnxDriverPlugin)
