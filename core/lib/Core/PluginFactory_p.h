#ifndef LANGCORE_PLUGINFACTORY_P_H
#define LANGCORE_PLUGINFACTORY_P_H

#include <map>
#include <shared_mutex>
#include <unordered_set>

#include <stdcorelib/3rdparty/llvm/smallvector.h>
#include <stdcorelib/support/sharedlibrary.h>

#include <LangCore/Core/PluginFactory.h>

namespace LangCore
{
    class LANGCORE_EXPORT PluginFactory::Impl {
    public:
        explicit Impl(PluginFactory *decl);
        virtual ~Impl();

        using Decl = PluginFactory;
        PluginFactory *_decl;

        void scanPlugins(const char *iid) const;

        std::map<std::string, llvm::SmallVector<std::filesystem::path>, std::less<>> pluginDirs;
        std::unordered_set<Plugin *> runtimePlugins;
        mutable std::unordered_set<std::string> scannedPluginDirs;  // 已扫描的插件目录缓存
        mutable std::map<std::filesystem::path::string_type, stdc::SharedLibrary *, std::less<>> libraryInstances;
        mutable std::unordered_set<std::string> pluginsDirty;
        mutable std::map<std::string, std::map<std::string, Plugin *>, std::less<>> allPlugins;
        mutable std::shared_mutex plugins_mtx;
    };

} // namespace LangCore

#endif // LANGCORE_PLUGINFACTORY_P_H
