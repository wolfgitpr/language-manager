#ifndef LANGMGR_ONNXDRIVER_SESSIONIMAGE_P_H
#define LANGMGR_ONNXDRIVER_SESSIONIMAGE_P_H

#include <filesystem>


#include <onnxruntime_cxx_api.h>

namespace LangPlugins::onnxdriver
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

} // namespace LangPlugins::onnxdriver

#endif // LANGMGR_ONNXDRIVER_SESSIONIMAGE_P_H
