#ifndef LANGCORE_PLUGINFACTORY_H
#define LANGCORE_PLUGINFACTORY_H

#include <filesystem>
#include <vector>

#include <stdcorelib/adt/array_view.h>

#include <LangCore/Core/Plugin.h>
#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{
    class LANGCORE_EXPORT PluginFactory {
    public:
        PluginFactory();
        virtual ~PluginFactory();

        void addRuntimePlugin(Plugin *plugin);
        std::vector<Plugin *> runtimePlugins() const;

        void addPluginPath(const char *iid, const std::filesystem::path &path);
        void setPluginPaths(const char *iid, stdc::array_view<std::filesystem::path> paths);
        std::vector<std::filesystem::path> pluginPaths(const char *iid) const;

        Plugin *plugin(const char *iid, const char *key) const;
        std::vector<Plugin *> plugins(const char *iid) const;

        template <class T>
        T *plugin(const char *key) const;
        template <class T>
        T *plugin(const char *iid, const char *key) const;
        template <class T>
        std::vector<T *> plugins(const char *iid) const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        explicit PluginFactory(Impl &impl);

        STDCORELIB_DISABLE_COPY_MOVE(PluginFactory);
    };

    template <class T>
    T *PluginFactory::plugin(const char *key) const {
        static_assert(std::is_base_of_v<Plugin, T>, "T should inherit from LangCore::Plugin");
        return static_cast<T *>(plugin(reinterpret_cast<T *>(0)->T::iid(), key));
    }

    template <class T>
    T *PluginFactory::plugin(const char *iid, const char *key) const {
        static_assert(std::is_base_of_v<Plugin, T>, "T should inherit from LangCore::Plugin");
        return static_cast<T *>(plugin(iid, key));
    }

    template <class T>
    std::vector<T *> PluginFactory::plugins(const char *iid) const {
        static_assert(std::is_base_of_v<Plugin, T>, "T should inherit from LangCore::Plugin");
        auto all = plugins(iid);
        std::vector<T *> result;
        result.reserve(all.size());
        for (auto *p : all)
            result.push_back(static_cast<T *>(p));
        return result;
    }

} // namespace LangCore

#endif // LANGCORE_PLUGINFACTORY_H
