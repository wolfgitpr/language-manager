#ifndef LANGPLUGINS_ONNXDRIVER_ENV_H
#define LANGPLUGINS_ONNXDRIVER_ENV_H

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <atomic>
#include <shared_mutex>

namespace LangPlugins::onnxDriver
{

    class Env {
    public:
        struct DeviceConfig {
            DeviceConfig() : ep(Api::Onnx::L1::CPUExecutionProvider), deviceIndex(-1) {}
            DeviceConfig(const Api::Onnx::L1::ExecutionProvider provider, const int index) :
                ep(provider), deviceIndex(index) {}

            Api::Onnx::L1::ExecutionProvider ep;
            int deviceIndex;
        };

        // Set/Get the entire device config atomically
        static void setDeviceConfig(const DeviceConfig &config);
        static DeviceConfig getDeviceConfig();
        static int64_t nextId();

    private:
        static inline DeviceConfig s_deviceConfig;
        static inline std::shared_mutex s_mutex;
        static inline std::atomic<int64_t> s_idCounter = 0;
    };

} // namespace LangPlugins::onnxDriver

#endif // LANGPLUGINS_ONNXDRIVER_ENV_H
