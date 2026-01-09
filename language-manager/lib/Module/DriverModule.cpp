#include "DriverModule.h"

#include <fstream>

#include <stdcorelib/path.h>
#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class DriverDefinition::Impl : public ModuleDefinition::Impl {
    public:
        Impl(const std::string &category) : ModuleDefinition::Impl(category) {}
    };

    class DriverCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(DriverCategory *decl, const std::string &category, Manager *su) :
            ModuleCategory::Impl(decl, category, su) {}

        ~Impl() override = default;
    };

    DriverDefinition::~DriverDefinition() = default;


    DriverDefinition::DriverDefinition() : ModuleDefinition(*new Impl(this->category())) {}

    DriverCategory::~DriverCategory() = default;

    std::string DriverCategory::key() const { return "driver"; }
    std::string DriverCategory::category() const { return "driver"; }

    DriverCategory::DriverCategory(Manager *env) : ModuleCategory(DriverCategory::key(), env) {}

    static ModuleCategoryRegistrar<DriverCategory> registrar;

} // namespace LangMgr
