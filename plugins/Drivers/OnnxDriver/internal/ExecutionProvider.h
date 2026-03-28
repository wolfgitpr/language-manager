#ifndef LANGPLUGINS_ONNXDRIVER_EXECUTIONPROVIDER_P_H
#define LANGPLUGINS_ONNXDRIVER_EXECUTIONPROVIDER_P_H

#include <onnxruntime_cxx_api.h>

namespace LangPlugins::OnnxDriver::V1
{

    bool initCUDA(const Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

    bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

} // namespace LangPlugins::OnnxDriver::V1

#endif // LANGPLUGINS_ONNXDRIVER_EXECUTIONPROVIDER_P_H
