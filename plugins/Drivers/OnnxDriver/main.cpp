#include <LangCore/Task/TaskPlugin.h>
#include "OnnxDriver.h"

using namespace LangCore;
using namespace LangPlugins::OnnxDriver::V1;

LANGCORE_DEFINE_DRIVER_PLUGIN(
    OnnxDriverPlugin,
    OnnxDriver,
    "onnx",
    1
)
