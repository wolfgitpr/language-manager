#ifndef LANGMGR_ENGINEMODULE_H
#define LANGMGR_ENGINEMODULE_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
{
    class EngineCategory;

    class EngineSpec : public ModuleSpec {
    public:
        ~EngineSpec() override;

    protected:
        class Impl;
        EngineSpec();

        friend class EngineCategory;
    };

    class EngineCategory : public ModuleCategory {
    public:
        ~EngineCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit EngineCategory(PackageManager *env);

        friend class PackageManager;
        friend class ModuleCategoryRegistrar<EngineCategory>;
    };

} // namespace LangMgr

#endif // LANGMGR_ENGINEMODULE_H
