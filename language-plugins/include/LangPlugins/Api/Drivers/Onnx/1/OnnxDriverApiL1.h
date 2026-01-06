#ifndef LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H
#define LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H

#include <filesystem>
#include <map>
#include <set>

#include <LangPlugins/Core/Tensor.h>
#include <LangPlugins/Inference/InferenceDriver.h>
#include <LangPlugins/Inference/InferenceSession.h>

namespace LangPlugins::Api::Onnx::L1
{

    inline constexpr char API_NAME[] = "onnx";

    inline constexpr int API_VERSION = 1;

    enum ExecutionProvider {
        CPUExecutionProvider = 0,
        CUDAExecutionProvider,
        DMLExecutionProvider,
        CoreMLExecutionProvider,
    };

    class DriverInitArgs : public InferenceDriverInitArgs {
    public:
        DriverInitArgs() : InferenceDriverInitArgs(API_NAME, API_VERSION) {}

        /// Load from progress
        bool loadFromProgress = false;

        /// The execution provider to use.
        ExecutionProvider ep = CPUExecutionProvider;

        /// The device index to use for CUDAExecutionProvider. (-1 means auto-select)
        int deviceIndex = -1;

        /// The onnxruntime library directory. (empty means use the default)
        std::filesystem::path runtimePath;
    };

    class SessionOpenArgs : public InferenceSessionOpenArgs {
    public:
        SessionOpenArgs() : InferenceSessionOpenArgs(API_NAME, API_VERSION) {}

        /// Whether to force the use of the CPU for the session.
        bool useCpu = false;
    };

    class SessionStartInput : public InferenceSessionStartInput {
    public:
        SessionStartInput() : InferenceSessionStartInput(API_NAME, API_VERSION) {}

        /// The input port names and the input tensors.
        std::map<std::string, LangMgr::NO<ITensor>> inputs;

        /// The output port names.
        std::set<std::string> outputs;
    };

    class SessionResult : public InferenceSessionResult {
    public:
        SessionResult() : InferenceSessionResult(API_NAME, API_VERSION) {}

        std::map<std::string, LangMgr::NO<ITensor>> outputs;
    };

} // namespace LangPlugins::Api::Onnx::L1

#endif // LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H
