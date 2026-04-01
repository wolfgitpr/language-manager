#ifndef LANGPLUGINS_ONNXDRIVER_SESSIONIMAGE_P_H
#define LANGPLUGINS_ONNXDRIVER_SESSIONIMAGE_P_H

#include <filesystem>

#include <onnxruntime_cxx_api.h>

namespace LangPlugins::OnnxDriver::V1
{

    class SessionImage {
    public:
        SessionImage();
        ~SessionImage();

        bool open(const std::filesystem::path &onnxPath, int hints, std::string *errorMessage = nullptr);

        std::vector<std::string> inputNames;
        std::vector<std::string> outputNames;

        Ort::Env env;
        Ort::Session session;
    };

} // namespace LangPlugins::OnnxDriver::V1

#endif // LANGPLUGINS_ONNXDRIVER_SESSIONIMAGE_P_H
