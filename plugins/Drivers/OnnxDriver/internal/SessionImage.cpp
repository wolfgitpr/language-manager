#include "SessionImage.h"

#include <onnxruntime_cxx_api.h>

#include <LangCore/Task/SessionTask.h>

#include "Env.h"
#include "ExecutionProvider.h"
#include "OnnxDriver_Logger.h"
#include "Session.h"

namespace LangPlugins::OnnxDriver::V1
{
    static Ort::Session createOrtSession(const Ort::Env &ortEnv, const std::filesystem::path &modelPath,
                                         const bool preferCpu, std::string *errorMessage) {
        const auto devConfig = Env::getDeviceConfig();
        const auto ep = devConfig.ep;
        auto deviceIndex = devConfig.deviceIndex;
        try {
            Ort::SessionOptions sessOpt;

            std::string initEPErrorMsg;
            if (!preferCpu) {
                switch (ep) {
                case LangCore::ExecutionProvider::DMLExecutionProvider:
                    {
                        if (!initDirectML(sessOpt, deviceIndex, &initEPErrorMsg)) {
                            // log warning: "Could not initialize DirectML: {initEPErrorMsg},
                            // falling back to CPU."
                            Log.langCoreWarning("Could not initialize DirectML: %1, falling back to CPU.",
                                                initEPErrorMsg);
                        } else {
                            Log.langCoreInfo("Use DirectML. Device index: %1", deviceIndex);
                        }
                        break;
                    }
                case LangCore::ExecutionProvider::CUDAExecutionProvider:
                    {
                        if (!initCUDA(sessOpt, deviceIndex, &initEPErrorMsg)) {
                            // log warning: "Could not initialize CUDA: {initEPErrorMsg}, falling
                            // back to CPU."
                            Log.langCoreWarning("Could not initialize CUDA: %1, falling back to CPU.", initEPErrorMsg);
                        } else {
                            Log.langCoreInfo("Use CUDA. Device index: %1", deviceIndex);
                        }
                        break;
                    }
                default:
                    {
                        // log info: "Use CPU."
                        Log.langCoreInfo("Use CPU.");
                        break;
                    }
                }
            } else {
                Log.langCoreInfo("The model prefers to use CPU. [%1]", modelPath.filename());
            }
            return Ort::Session{ortEnv, std::filesystem::path::string_type(modelPath).c_str(), sessOpt};
        }
        catch (const Ort::Exception &e) {
            if (errorMessage) {
                *errorMessage = e.what();
            }
        }
        return Ort::Session{nullptr};
    }

    static void loggingFuncOrt(void *param, const OrtLoggingLevel severity, const char *category, const char *logId,
                               const char *code_location, const char *message) {
        switch (severity) {
        case ORT_LOGGING_LEVEL_VERBOSE:
            Log.langCoreLog(Debug, "[%1] %2", code_location, message);
            break;
        case ORT_LOGGING_LEVEL_WARNING:
            Log.langCoreLog(Warning, "[%1] %2", code_location, message);
            break;
        case ORT_LOGGING_LEVEL_ERROR:
            Log.langCoreLog(Critical, "[%1] %2", code_location, message);
            break;
        case ORT_LOGGING_LEVEL_FATAL:
            Log.langCoreLog(Fatal, "[%1] %2", code_location, message);
            break;
        default:
            Log.langCoreLog(Information, "[%1] %2", code_location, message);
            break;
        }
    }

    SessionImage::SessionImage() :
        env(ORT_LOGGING_LEVEL_WARNING, "langCore", loggingFuncOrt, nullptr), session(nullptr) {}

    SessionImage::~SessionImage() = default;

    bool SessionImage::open(const std::filesystem::path &onnxPath, const int hints, std::string *errorMessage) {
        auto filename = onnxPath.filename();
        Log.langCoreDebug("SessionImage [%1] - creating", filename);

        session = createOrtSession(env, onnxPath, hints & Session::SH_PreferCPUHint, errorMessage);
        if (!session) {
            Log.langCoreCritical("SessionImage [%1] - create failed", filename);
            return false;
        }
        const Ort::AllocatorWithDefaultOptions allocator;

        const auto inputCount = session.GetInputCount();
        inputNames.reserve(inputCount);
        for (size_t i = 0; i < inputCount; ++i) {
            inputNames.emplace_back(session.GetInputNameAllocated(i, allocator).get());
        }

        const auto outputCount = session.GetOutputCount();
        outputNames.reserve(outputCount);
        for (size_t i = 0; i < outputCount; ++i) {
            outputNames.emplace_back(session.GetOutputNameAllocated(i, allocator).get());
        }
        Log.langCoreDebug("SessionImage [%1] - created successfully", filename);
        return true;
    }

} // namespace LangPlugins::OnnxDriver::V1
