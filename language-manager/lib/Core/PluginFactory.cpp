#include "../../include/LangMgr/Core/PluginFactory.h"
#include "PluginFactory_p.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <utility>

#include <LangMgr/Support/JSON.h>

#include <stdcorelib/3rdparty/llvm/smallvector.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

namespace fs = std::filesystem;

namespace LangMgr
{
    PluginFactory::Impl::Impl(PluginFactory *decl) : _decl(decl) {}

    PluginFactory::Impl::~Impl() {
        // Unload all libraries
        for (const auto &[fst, snd] : std::as_const(libraryInstances)) {
            delete snd;
        }
    }

    // Helper function to parse desc.json
    struct PluginDesc {
        std::string target;
        bool valid = false;
    };

    PluginDesc parsePluginDesc(const fs::path &descPath) {
        PluginDesc desc;

        try {
            std::ifstream ifs(descPath);
            if (!ifs.is_open())
                return desc;

            const std::string jsonStr((std::istreambuf_iterator(ifs)), (std::istreambuf_iterator<char>()));

            // parse JSON
            std::string jsonErrorMessage;
            const JsonValue jsonDoc = JsonValue::fromJson(jsonStr, true, &jsonErrorMessage);
            if (!jsonErrorMessage.empty())
                return desc;
            if (!jsonDoc.isObject())
                return desc;
            const auto &docObj = jsonDoc.toObject();

            const auto it = docObj.find("target");
            if (it == docObj.end())
                return desc;
            desc.target = it->second.toString();
            desc.valid = true;
        }
        catch (const std::exception &e) {
            std::cerr << "Failed to parse plugin desc.json: " << descPath << ", error: " << e.what() << std::endl;
        }

        return desc;
    }

    void PluginFactory::Impl::scanPlugins(const char *iid) const {
        auto &plugins = allPlugins[iid];

        // Add runtime plugins
        for (const auto &plugin : runtimePlugins) {
            if (strcmp(iid, plugin->iid()) == 0) {
                std::ignore = plugins.insert(std::make_pair(plugin->key(), plugin));
            }
        }

        if (const auto it = pluginDirs.find(iid); it != pluginDirs.end()) {
            for (const auto &pluginDir : it->second) {
                fs::path descPath = pluginDir / "plugin.json";
                if (!fs::exists(descPath))
                    continue;

                // Parse desc.json
                auto [target, valid] = parsePluginDesc(descPath);
                if (!valid) {
                    std::cerr << "Invalid plugin.json in: " << pluginDir << std::endl;
                    continue;
                }

                // Construct dll path
                fs::path dllPath = pluginDir / target;
                if (!fs::exists(dllPath)) {
                    std::cerr << "Plugin dll not found: " << dllPath << std::endl;
                    continue;
                }

                // Check if already loaded
                if (libraryInstances.count(dllPath) || !stdc::SharedLibrary::isLibrary(dllPath))
                    continue;

                stdc::SharedLibrary so;
                stdc::SharedLibrary::setLibraryPath(pluginDir);
                if (!so.open(dllPath)) {
                    std::cout << "path: " << dllPath << "\nerror: " << so.lastError() << std::endl;
                    continue;
                }

                using PluginGetter = Plugin *(*)();
                const auto getter = reinterpret_cast<PluginGetter>(so.resolve("langMgr_plugin_instance"));
                if (!getter) {
                    std::cerr << "Failed to resolve plugin instance function in: " << dllPath << std::endl;
                    continue;
                }

                if (auto plugin = getter(); !plugin || strcmp(iid, plugin->iid()) != 0 ||
                    !plugins.insert(std::make_pair(plugin->key(), plugin)).second) {
                    std::cerr << "Failed to load plugin or IID mismatch: " << dllPath << std::endl;
                    continue;
                } else {
                    std::cout << "Successfully loaded plugin: " << pluginDir << " (target: " << target << ")"
                              << std::endl;
                    std::cout << "iid: " << iid << "; key: " << plugin->key() << std::endl << std::endl;
                }
                libraryInstances[dllPath] = new stdc::SharedLibrary(std::move(so));
            }
        }

        if (plugins.empty()) {
            allPlugins.erase(iid);
        }
    }

    PluginFactory::PluginFactory() : _impl(new Impl(this)) {}

    PluginFactory::~PluginFactory() = default;

    void PluginFactory::addRuntimePlugin(Plugin *plugin) {
        __stdc_impl_t;
        std::unique_lock lock(impl.plugins_mtx);
        impl.runtimePlugins.emplace(plugin);
        impl.pluginsDirty.insert(plugin->iid());
    }

    std::vector<Plugin *> PluginFactory::runtimePlugins() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.plugins_mtx);
        return {impl.runtimePlugins.begin(), impl.runtimePlugins.end()};
    }

    void PluginFactory::addPluginPath(const char *iid, const std::filesystem::path &path) {
        __stdc_impl_t;
        if (!fs::is_directory(path)) {
            return;
        }

        std::unique_lock lock(impl.plugins_mtx);
        const fs::path canonicalPath = fs::canonical(path);

        // Scan subdirectories for plugins
        for (const auto &entry : fs::directory_iterator(canonicalPath)) {
            if (!entry.is_directory()) {
                continue;
            }

            const auto &pluginDir = fs::canonical(entry.path());

            if (fs::path descPath = pluginDir / "plugin.json"; fs::exists(descPath)) {
                // This is a plugin directory
                impl.pluginDirs[iid].push_back(pluginDir);
            }
        }

        impl.pluginsDirty.insert(iid);
    }

    void PluginFactory::setPluginPaths(const char *iid, const stdc::array_view<std::filesystem::path> paths) {
        __stdc_impl_t;
        std::unique_lock lock(impl.plugins_mtx);

        // Clear existing paths for this IID
        impl.pluginDirs.erase(iid);

        if (!paths.empty()) {
            llvm::SmallVector<fs::path> pluginDirs;

            for (const auto &path : paths) {
                if (!fs::is_directory(path)) {
                    continue;
                }

                fs::path canonicalPath = fs::canonical(path);

                // Scan subdirectories for plugins
                for (const auto &entry : fs::directory_iterator(canonicalPath)) {
                    if (!entry.is_directory()) {
                        continue;
                    }

                    const auto &pluginDir = fs::canonical(entry.path());

                    if (fs::path descPath = pluginDir / "plugin.json"; fs::exists(descPath)) {
                        // This is a plugin directory
                        pluginDirs.push_back(pluginDir);
                    }
                }
            }

            if (!pluginDirs.empty()) {
                impl.pluginDirs[iid] = pluginDirs;
            }
        }

        impl.pluginsDirty.insert(iid);
    }

    std::vector<std::filesystem::path> PluginFactory::pluginPaths(const char *iid) const {
        __stdc_impl_t;

        std::shared_lock lock(impl.plugins_mtx);
        const auto it = impl.pluginDirs.find(iid);
        if (it == impl.pluginDirs.end()) {
            return {};
        }
        return {it->second.begin(), it->second.end()};
    }

    Plugin *PluginFactory::plugin(const char *iid, const char *key) const {
        __stdc_impl_t;

        std::unique_lock lock(impl.plugins_mtx);
        if (impl.pluginsDirty.count(iid)) {
            impl.scanPlugins(iid);
        }

        const auto it = impl.allPlugins.find(iid);
        if (it == impl.allPlugins.end()) {
            return nullptr;
        }

        const auto &pluginsMap = it->second;
        const auto it2 = pluginsMap.find(key);
        if (it2 == pluginsMap.end()) {
            return nullptr;
        }
        return it2->second;
    }

    std::vector<Plugin *> PluginFactory::plugins(const char *iid) const {
        __stdc_impl_t;

        std::unique_lock lock(impl.plugins_mtx);
        if (impl.pluginsDirty.count(iid))
            impl.scanPlugins(iid);

        const auto it = impl.allPlugins.find(iid);
        if (it == impl.allPlugins.end())
            return {};

        const auto &pluginsMap = it->second;
        std::vector<Plugin *> pluginsVec;
        for (const auto &[fst, snd] : pluginsMap)
            pluginsVec.push_back(snd);
        return pluginsVec;
    }

    /*!
        \internal
    */
    PluginFactory::PluginFactory(Impl &impl) : _impl(&impl) {}

} // namespace LangMgr
