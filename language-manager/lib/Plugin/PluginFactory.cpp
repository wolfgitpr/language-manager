#include "PluginFactory.h"
#include "PluginFactory_p.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <utility>

#include <stdcorelib/3rdparty/llvm/smallvector.h>
#include <stdcorelib/pimpl.h>

namespace fs = std::filesystem;

namespace LangMgr
{

    using StaticPluginMap = std::map<std::string, llvm::SmallVector<StaticPlugin, 10>>;

    static StaticPluginMap &getStaticPluginMap() {
        static StaticPluginMap staticPluginMap;
        return staticPluginMap;
    }

    void StaticPlugin::registerStaticPlugin(const char *pluginSet, StaticPlugin plugin) {
        auto &plugins = getStaticPluginMap()[pluginSet];

        // insert the plugin in the list, sorted by address, so we can detect
        // duplicate registrations
        static const auto comparator = [=](const StaticPlugin &p1, const StaticPlugin &p2)
        {
            using Less = std::less<decltype(plugin.instance)>;
            return Less{}(p1.instance, p2.instance);
        };
        if (const auto pos = std::lower_bound(plugins.begin(), plugins.end(), plugin, comparator);
            pos == plugins.end() || pos->instance != plugin.instance)
            plugins.insert(pos, plugin);
    }

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
            std::ifstream file(descPath);
            if (!file.is_open()) {
                return desc;
            }

            nlohmann::json json;
            file >> json;

            if (json.contains("target") && json["target"].is_string()) {
                desc.target = json["target"].get<std::string>();
                desc.valid = true;
            }
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
                fs::path descPath = pluginDir / "desc.json";
                if (!fs::exists(descPath))
                    continue;

                // Parse desc.json
                auto [target, valid] = parsePluginDesc(descPath);
                if (!valid) {
                    std::cerr << "Invalid desc.json in: " << pluginDir << std::endl;
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

    std::vector<std::string> PluginFactory::staticPluginSets() {
        const auto &map = getStaticPluginMap();
        std::vector<std::string> pluginSets;
        pluginSets.reserve(map.size());
        for (const auto &[fst, snd] : map) {
            pluginSets.push_back(fst);
        }
        return pluginSets;
    }

    std::vector<StaticPlugin> PluginFactory::staticPlugins(const char *pluginSet) {
        auto &map = getStaticPluginMap();
        const auto it = map.find(pluginSet);
        if (it == map.end()) {
            return {};
        }
        return {it->second.begin(), it->second.end()};
    }

    std::vector<Plugin *> PluginFactory::staticInstances(const char *pluginSet) {
        auto &map = getStaticPluginMap();
        std::vector<Plugin *> instances;
        const auto it = map.find(pluginSet);
        if (it == map.end()) {
            return {};
        }
        const auto &plugins = it->second;
        instances.reserve(plugins.size());
        for (const StaticPlugin plugin : plugins)
            instances.push_back(plugin.instance());
        return instances;
    }

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

            if (fs::path descPath = pluginDir / "desc.json"; fs::exists(descPath)) {
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

                    if (fs::path descPath = pluginDir / "desc.json"; fs::exists(descPath)) {
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

    /*!
        \internal
    */
    PluginFactory::PluginFactory(Impl &impl) : _impl(&impl) {}

} // namespace LangMgr
