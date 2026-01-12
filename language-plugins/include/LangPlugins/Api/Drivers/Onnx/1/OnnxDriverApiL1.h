#ifndef LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H
#define LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H

#include <filesystem>
#include <map>
#include <set>

#include <LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

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

    class DriverInitArgs : public LangMgr::TaskInitArgs {
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

    class SessionOpenArgs : public LangMgr::TaskInitArgs {
    public:
        SessionOpenArgs() : TaskInitArgs(API_NAME, API_CLASS, API_LEVEL) {}

        /// Whether to force the use of the CPU for the session.
        bool useCpu = false;
    };

    class SessionStartInput : public LangMgr::TaskStartInput {
    public:
        SessionStartInput() : TaskStartInput(API_NAME, API_CLASS, API_LEVEL) {}

        /// The input port names and the input tensors.
        std::map<std::string, LangMgr::NO<ITensor>> inputs;

        /// The output port names.
        std::set<std::string> outputs;
    };

    class SessionResult : public LangMgr::TaskResult {
    public:
        SessionResult() : TaskResult(API_NAME, API_CLASS, API_LEVEL) {}

        std::map<std::string, LangMgr::NO<ITensor>> outputs;
    };

} // namespace LangPlugins::Api::Onnx::L1

#endif // LANGPLG_API_ONNX_ONNXDRIVERAPIL1_H
