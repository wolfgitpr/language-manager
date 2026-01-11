#ifndef LANGMGR_SPLITTERMODULE_H
#define LANGMGR_SPLITTERMODULE_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
{
    class SplitterTask;
    class SplitterCategory;

    class SplitterDefinition : public ModuleDefinition {
    public:
        ~SplitterDefinition() override;

    protected:
        class Impl;
        SplitterDefinition();

        friend class SplitterCategory;
    };

    class SplitterCategory : public ModuleCategory {
    public:
        ~SplitterCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit SplitterCategory(Manager *env);

        friend class Manager;
        friend class ModuleCategoryRegistrar<SplitterCategory>;
    };

} // namespace LangMgr

#endif // LANGMGR_SPLITTERMODULE_H
