#ifndef LANGCORE_DRIVERMODULE_H
#define LANGCORE_DRIVERMODULE_H

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/Task.h>

namespace LangCore
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

} // namespace LangCore
#endif // LANGCORE_DRIVERMODULE_H
