#ifndef LANGMGR_ONNXDRIVER_EXECUTIONPROVIDER_P_H
#define LANGMGR_ONNXDRIVER_EXECUTIONPROVIDER_P_H

#include <onnxruntime_cxx_api.h>

namespace LangPlugins::onnxDriver
{

    bool initCUDA(const Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

    bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

} // namespace LangPlugins::onnxDriver

#endif // LANGMGR_ONNXDRIVER_EXECUTIONPROVIDER_P_H
