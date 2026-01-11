#include "SplitterModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class SplitterDefinition::Impl : public ModuleDefinition::Impl {
    public:
        Impl(const std::string &category) : ModuleDefinition::Impl(category) {}
    };

    class SplitterCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(SplitterCategory *decl, const std::string &category, Manager *su) :
            ModuleCategory::Impl(decl, category, su) {}

        ~Impl() override = default;
    };

    SplitterDefinition::~SplitterDefinition() = default;


    SplitterDefinition::SplitterDefinition() : ModuleDefinition(*new Impl(this->category())) {}

    SplitterCategory::~SplitterCategory() = default;

    std::string SplitterCategory::key() const { return "splitter"; }
    std::string SplitterCategory::category() const { return "splitter"; }

    SplitterCategory::SplitterCategory(Manager *env) : ModuleCategory(SplitterCategory::category(), env) {}

    ModuleCategoryRegistrar<SplitterCategory> registrar;

} // namespace LangMgr
