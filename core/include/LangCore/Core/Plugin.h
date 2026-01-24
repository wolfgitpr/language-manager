#ifndef LANGCORE_PLUGIN_H
#define LANGCORE_PLUGIN_H

#include <filesystem>

#include <stdcorelib/support/sharedlibrary.h>

#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{

    class LANGCORE_EXPORT Plugin {
    public:
        virtual ~Plugin() = default;

        virtual const char *iid() const = 0;

        virtual const char *key() const = 0;

        std::filesystem::path path() const { return stdc::SharedLibrary::locateLibraryPath(this); }
    };
} // namespace LangCore

#define LANGCORE_EXPORT_PLUGIN(PLUGIN_NAME)                                                                            \
    extern "C" STDCORELIB_DECL_EXPORT LangCore::Plugin *langCore_plugin_instance() {                                   \
        static PLUGIN_NAME _instance;                                                                                  \
        return &_instance;                                                                                             \
    }
#endif // LANGCORE_MANAGER_PLUGIN_H
