#include "DriverModule.h"

#include <fstream>

#include <stdcorelib/path.h>
#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangCore
{

    class DriverSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class DriverCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(DriverCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    DriverSpec::~DriverSpec() = default;


    DriverSpec::DriverSpec() : ModuleSpec(*new Impl(this->category())) {}

    DriverCategory::~DriverCategory() = default;

    std::string DriverCategory::key() const { return "driver"; }
    std::string DriverCategory::category() const { return "driver"; }

    DriverCategory::DriverCategory(PackageManager *env) : ModuleCategory(DriverCategory::key(), env) {}

    static ModuleCategoryRegistrar<DriverCategory> registrar;

} // namespace LangCore
