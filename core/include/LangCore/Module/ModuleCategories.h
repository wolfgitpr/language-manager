#ifndef LANGCORE_MODULECATEGORIES_H
#define LANGCORE_MODULECATEGORIES_H

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    class DriverTask;
    class G2pTask;
    class SplitterTask;
    class TaggerTask;
    class CleanerTask;

    LANGCORE_DECLARE_MODULE_CATEGORY(Driver, "driver")
    LANGCORE_DECLARE_MODULE_CATEGORY(G2p, "g2p")
    LANGCORE_DECLARE_MODULE_CATEGORY(Splitter, "splitter")
    LANGCORE_DECLARE_MODULE_CATEGORY(Tagger, "tagger")
    LANGCORE_DECLARE_MODULE_CATEGORY(Cleaner, "cleaner")

} // namespace LangCore

#endif // LANGCORE_MODULECATEGORIES_H
