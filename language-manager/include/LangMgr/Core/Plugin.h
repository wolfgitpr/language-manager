#ifndef LANGUAGE_MANAGER_PLUGIN_H
#define LANGUAGE_MANAGER_PLUGIN_H

#include <filesystem>

#include <stdcorelib/support/sharedlibrary.h>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{

    class LANGMGR_EXPORT Plugin {
    public:
        virtual ~Plugin() = default;

        virtual const char *iid() const = 0;

        virtual const char *key() const = 0;

        std::filesystem::path path() const { return stdc::SharedLibrary::locateLibraryPath(this); }
    };
} // namespace LangMgr

#define LANGMGR_EXPORT_PLUGIN(PLUGIN_NAME)                                                                             \
    extern "C" STDCORELIB_DECL_EXPORT LangMgr::Plugin *langMgr_plugin_instance() {                                     \
        static PLUGIN_NAME _instance;                                                                                  \
        return &_instance;                                                                                             \
    }
#endif // LANGUAGE_MANAGER_PLUGIN_H
