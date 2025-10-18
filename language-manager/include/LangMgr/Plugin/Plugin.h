#ifndef LANGUAGE_MANAGER_PLUGIN_H
#define LANGUAGE_MANAGER_PLUGIN_H

#include <filesystem>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{

    /// Plugin - Base class for all plugins.
    class LANGMGR_EXPORT Plugin {
    public:
        virtual ~Plugin();

    public:
        /// Returns the interface identifier of the plugin.
        virtual const char *iid() const = 0;

        /// Returns the key of the plugin.
        virtual const char *key() const = 0;

    public:
        std::filesystem::path path() const;
    };

    class StaticPlugin {
    public:
        using PluginInstanceFunction = Plugin *(*)();

        constexpr StaticPlugin(PluginInstanceFunction i) : instance(i) {}

        PluginInstanceFunction instance = nullptr;

    public:
        LANGMGR_EXPORT static void registerStaticPlugin(const char *pluginSet, StaticPlugin plugin);
    };

} // namespace LangMgr

#define LANGMGR_EXPORT_PLUGIN(PLUGIN_NAME)                                                                            \
    extern "C" STDCORELIB_DECL_EXPORT LangMgr::Plugin *synthrt_plugin_instance() {                                     \
        static PLUGIN_NAME _instance;                                                                                  \
        return &_instance;                                                                                             \
    }

#define LANGMGR_EXPORT_STATIC_PLUGIN(PLUGIN_NAME, PLUGIN_SET)                                                         \
    struct initializer {                                                                                               \
        initializer() {                                                                                                \
            LangMgr::StaticPlugin::registerStaticPlugin(PLUGIN_SET,                                                    \
                                                        LangMgr::StaticPlugin(                                         \
                                                            []() -> LangMgr::Plugin *                                  \
                                                            {                                                          \
                                                                static PLUGIN_NAME _instance;                          \
                                                                return &_instance;                                     \
                                                            }));                                                       \
        }                                                                                                              \
        ~initializer() {}                                                                                              \
    } dummy;

#endif // LANGUAGE_MANAGER_PLUGIN_H
