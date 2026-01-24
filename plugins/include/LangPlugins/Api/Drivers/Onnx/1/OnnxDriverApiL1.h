#ifndef LANGPLUGINS_API_ONNX_ONNXDRIVERAPIL1_H
#define LANGPLUGINS_API_ONNX_ONNXDRIVERAPIL1_H

#include <filesystem>
#include <map>
#include <set>

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

namespace LangPlugins::Api::Onnx::L1
{

    constexpr char API_NAME[] = "Onnx";
    constexpr char API_CLASS[] = "driver.onnx.OnnxInference";
    constexpr int API_LEVEL = 1;

    enum ExecutionProvider {
        CPUExecutionProvider = 0,
        CUDAExecutionProvider,
        DMLExecutionProvider,
        CoreMLExecutionProvider,
    };

    class DriverInitArgs : public LangCore::TaskInitArgs {
    public:
        DriverInitArgs() : TaskInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        /// Load from progress
        bool loadFromProgress = false;

        /// The execution provider to use.
        ExecutionProvider ep = CPUExecutionProvider;

        /// The device index to use for CUDAExecutionProvider. (-1 means auto-select)
        int deviceIndex = -1;

        /// The onnxruntime library directory. (empty means use the default)
        std::filesystem::path runtimePath;
    };

    class SessionOpenArgs : public LangCore::TaskInitArgs {
    public:
        SessionOpenArgs() : TaskInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        /// Whether to force the use of the CPU for the session.
        bool useCpu = false;
    };

    class SessionStartInput : public LangCore::TaskStartInput {
    public:
        SessionStartInput() : TaskStartInput(API_NAME, API_CLASS, API_LEVEL) {}

        /// The input port names and the input tensors.
        std::map<std::string, LangCore::NO<ITensor>> inputs;

        /// The output port names.
        std::set<std::string> outputs;
    };

    class SessionResult : public LangCore::TaskResult {
    public:
        SessionResult() : TaskResult(API_NAME, API_CLASS, API_LEVEL) {}

        std::map<std::string, LangCore::NO<ITensor>> outputs;
    };

} // namespace LangPlugins::Api::Onnx::L1

#endif // LANGPLUGINS_API_ONNX_ONNXDRIVERAPIL1_H
