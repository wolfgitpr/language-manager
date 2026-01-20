#ifndef LANGMGR_TAGGERMODULE_H
#define LANGMGR_TAGGERMODULE_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
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

} // namespace LangMgr

#endif // LANGMGR_TAGGERMODULE_H
