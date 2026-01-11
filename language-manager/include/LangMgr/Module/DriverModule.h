#ifndef LANGMGR_DRIVERMODULE_H
#define LANGMGR_DRIVERMODULE_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Task/Task.h>


namespace LangMgr
{
    class DriverTask;
    class DriverCategory;

    class DriverDefinition : public ModuleDefinition {
    public:
        ~DriverDefinition() override;

    protected:
        class Impl;
        DriverDefinition();

        friend class DriverCategory;
    };

    class SessionFactory;

    class DriverCategory : public ModuleCategory {
    public:
        ~DriverCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit DriverCategory(Manager *env);

        friend class Manager;
        friend class ModuleCategoryRegistrar<DriverCategory>;
    };

} // namespace LangMgr
#endif // LANGMGR_DRIVERMODULE_H
