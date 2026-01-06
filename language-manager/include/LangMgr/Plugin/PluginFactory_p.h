#ifndef LANGUAGE_MANAGER_PLUGINFACTORY_P_H
#define LANGUAGE_MANAGER_PLUGINFACTORY_P_H

#include <map>
#include <shared_mutex>
#include <unordered_set>

#include <stdcorelib/3rdparty/llvm/smallvector.h>
#include <stdcorelib/support/sharedlibrary.h>

#include <LangMgr/Plugin/PluginFactory.h>

namespace LangMgr
{

    class PluginFactory::Impl {
    public:
        explicit Impl(PluginFactory *decl);
        virtual ~Impl();

        using Decl = PluginFactory;
        PluginFactory *_decl;

        void scanPlugins(const char *iid) const;

        std::map<std::string, llvm::SmallVector<std::filesystem::path>, std::less<>> pluginDirs;
        std::unordered_set<Plugin *> runtimePlugins;
        mutable std::map<std::filesystem::path::string_type, stdc::SharedLibrary *, std::less<>> libraryInstances;
        mutable std::unordered_set<std::string> pluginsDirty;
        mutable std::map<std::string, std::map<std::string, Plugin *>, std::less<>> allPlugins;
        mutable std::shared_mutex plugins_mtx;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_PLUGINFACTORY_P_H
