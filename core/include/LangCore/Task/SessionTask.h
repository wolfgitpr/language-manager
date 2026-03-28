#ifndef LANGUAGE_MANAGER_SESSIONTASK_H
#define LANGUAGE_MANAGER_SESSIONTASK_H

#include <filesystem>
#include <map>
#include <set>

#include <LangCore/Support/Tensor.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    enum ExecutionProvider {
        CPUExecutionProvider = 0,
        CUDAExecutionProvider,
        DMLExecutionProvider,
        CoreMLExecutionProvider,
    };

    class DriverInitArgs : public TaskInitArgs {
    public:
        DriverInitArgs() {}

        /// Load from progress
        bool loadFromProcess = false;

        /// The execution provider to use.
        ExecutionProvider ep = CPUExecutionProvider;

        /// The device index to use for CUDAExecutionProvider. (-1 means auto-select)
        int deviceIndex = -1;

        /// The onnxruntime library directory. (empty means use the default)
        std::filesystem::path runtimePath;
    };

    class SessionOpenArgs : public TaskInitArgs {
    public:
        SessionOpenArgs() {}

        /// Whether to force the use of the CPU for the session.
        bool useCpu = false;
    };

    class SessionStartInput : public TaskInput {
    public:
        SessionStartInput() {}

        /// The input port names and the input tensors.
        std::map<std::string, NO<ITensor>> inputs;

        /// The output port names.
        std::set<std::string> outputs;
    };

    class SessionResult : public TaskResult {
    public:
        SessionResult() {}

        std::map<std::string, NO<ITensor>> outputs;
    };

} // namespace LangCore
#endif // LANGUAGE_MANAGER_SESSIONTASK_H
