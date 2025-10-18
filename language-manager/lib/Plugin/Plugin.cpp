#include "Plugin.h"

#include <stdcorelib/support/sharedlibrary.h>

namespace LangMgr
{

    Plugin::~Plugin() = default;

    std::filesystem::path Plugin::path() const { return stdc::SharedLibrary::locateLibraryPath(this); }

} // namespace LangMgr
