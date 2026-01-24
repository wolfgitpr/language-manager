#ifndef LANGCORE_TAGGERMODULE_H
#define LANGCORE_TAGGERMODULE_H

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>

namespace LangCore
{
    class TaggerTask;

    class TaggerSpec : public ModuleSpec {
    public:
        ~TaggerSpec() override;

    protected:
        class Impl;
        TaggerSpec();

        friend class TaggerCategory;
    };

    class TaggerCategory : public ModuleCategory {
    public:
        ~TaggerCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit TaggerCategory(PackageManager *env);

        friend class PackageManager;
        friend class ModuleCategoryRegistrar<TaggerCategory>;
    };

} // namespace LangCore

#endif // LANGCORE_TAGGERMODULE_H
