#ifndef LANGCORE_ENGINEMODULE_H
#define LANGCORE_ENGINEMODULE_H

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>

namespace LangCore
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

} // namespace LangCore

#endif // LANGCORE_ENGINEMODULE_H
