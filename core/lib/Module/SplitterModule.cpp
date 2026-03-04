#include "SplitterModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangCore
{

    class SplitterSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class SplitterCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(SplitterCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    SplitterSpec::~SplitterSpec() = default;

    SplitterSpec::SplitterSpec() : ModuleSpec(*new Impl(this->category())) {}

    SplitterCategory::~SplitterCategory() = default;

    std::string SplitterCategory::key() const { return "splitter"; }
    std::string SplitterCategory::category() const { return "splitter"; }

    SplitterCategory::SplitterCategory(PackageManager *env) : ModuleCategory(SplitterCategory::category(), env) {}

    static ModuleCategoryRegistrar<SplitterCategory> registrar;

} // namespace LangCore
