#ifndef LANGCORE_SPLITTERMODULE_H
#define LANGCORE_SPLITTERMODULE_H

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>

namespace LangCore
{
    class SplitterTask;

    class SplitterSpec : public ModuleSpec {
    public:
        ~SplitterSpec() override;

    protected:
        class Impl;
        SplitterSpec();

        friend class SplitterCategory;
    };

    class SplitterCategory : public ModuleCategory {
    public:
        ~SplitterCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit SplitterCategory(PackageManager *env);

        friend class PackageManager;
        friend class ModuleCategoryRegistrar<SplitterCategory>;
    };

} // namespace LangCore

#endif // LANGCORE_SPLITTERMODULE_H
