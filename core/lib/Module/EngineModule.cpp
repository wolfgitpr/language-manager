#include "EngineModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangCore
{

    class EngineSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class EngineCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(EngineCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    EngineSpec::~EngineSpec() = default;


    EngineSpec::EngineSpec() : ModuleSpec(*new Impl(this->category())) {}

    EngineCategory::~EngineCategory() = default;

    std::string EngineCategory::key() const { return "engine"; }
    std::string EngineCategory::category() const { return "engine"; }

    EngineCategory::EngineCategory(PackageManager *env) : ModuleCategory(EngineCategory::category(), env) {}

    ModuleCategoryRegistrar<EngineCategory> registrar;

} // namespace LangCore
