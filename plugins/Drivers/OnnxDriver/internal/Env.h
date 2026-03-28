#ifndef LANGPLUGINS_ONNXDRIVER_ENV_H
#define LANGPLUGINS_ONNXDRIVER_ENV_H

#include <atomic>
#include <shared_mutex>

#include <LangCore/Task/SessionTask.h>

namespace LangPlugins::OnnxDriver::V1
{

    class Env {
    public:
        struct DeviceConfig {
            DeviceConfig() : ep(LangCore::CPUExecutionProvider), deviceIndex(-1) {}
            DeviceConfig(const LangCore::ExecutionProvider provider, const int index) :
                ep(provider), deviceIndex(index) {}

            LangCore::ExecutionProvider ep;
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

} // namespace LangPlugins::OnnxDriver::V1

#endif // LANGPLUGINS_ONNXDRIVER_ENV_H
