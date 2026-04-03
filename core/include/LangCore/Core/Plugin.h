#ifndef LANGCORE_PLUGIN_H
#define LANGCORE_PLUGIN_H

#include <stdcorelib/support/sharedlibrary.h>
#include <stdcorelib/stdc_global.h>
#include <filesystem>

#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{

    class LANGCORE_EXPORT Plugin {
    public:
        virtual ~Plugin() = default;

        virtual const char *iid() const = 0;

        virtual const char *key() const = 0;

        virtual int apiLevel() const = 0;

        std::filesystem::path path() const { return stdc::SharedLibrary::locateLibraryPath(this); }
    };
} // namespace LangCore

#define LANGCORE_EXPORT_PLUGIN(PLUGIN_NAME)                                                                            \
    extern "C" STDCORELIB_DECL_EXPORT LangCore::Plugin *langCore_plugin_instance() {                                   \
        static PLUGIN_NAME _instance;                                                                                  \
        return &_instance;                                                                                             \
    }

// 简化 TaskPlugin 定义的宏
// 注意：使用此宏前必须先包含 <LangCore/Task/TaskPlugin.h>
#define LANGCORE_DEFINE_TASK_PLUGIN(PluginClass, TaskClass, PluginKey, ApiLevel)                                      \
    class PluginClass final : public TaskPlugin {                                                                     \
    public:                                                                                                            \
        PluginClass() = default;                                                                                       \
        int apiLevel() const override { return ApiLevel; }                                                             \
        const char *key() const override { return PluginKey; }                                                         \
        Expected<NO<Task>> createTask(const ModuleSpec *spec) override {                                               \
            return NO<TaskClass>::create(spec);                                                                        \
        }                                                                                                              \
    };                                                                                                                 \
    LANGCORE_EXPORT_PLUGIN(PluginClass)

// 简化 DriverPlugin 定义的宏
// 注意：使用此宏前必须先包含 <LangCore/Task/TaskPlugin.h>
#define LANGCORE_DEFINE_DRIVER_PLUGIN(PluginClass, FactoryClass, PluginKey, ApiLevel)                                  \
    class PluginClass final : public DriverPlugin {                                                                   \
    public:                                                                                                            \
        PluginClass() = default;                                                                                       \
        int apiLevel() const override { return ApiLevel; }                                                             \
        const char *key() const override { return PluginKey; }                                                         \
        Expected<NO<SessionFactory>> create() override {                                                               \
            return NO<FactoryClass>::create();                                                                         \
        }                                                                                                              \
    };                                                                                                                 \
    LANGCORE_EXPORT_PLUGIN(PluginClass)

#endif // LANGCORE_PLUGIN_H
