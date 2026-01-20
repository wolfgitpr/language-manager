#ifndef LANGMGR_DRIVERMODULE_H
#define LANGMGR_DRIVERMODULE_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Task/Task.h>


namespace LangMgr
{
    class DriverTask;

    class DriverSpec : public ModuleSpec {
    public:
        ~DriverSpec() override;

    protected:
        class Impl;
        DriverSpec();

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
        explicit DriverCategory(PackageManager *env);

        friend class PackageManager;
        friend class ModuleCategoryRegistrar<DriverCategory>;
    };

} // namespace LangMgr
#endif // LANGMGR_DRIVERMODULE_H
