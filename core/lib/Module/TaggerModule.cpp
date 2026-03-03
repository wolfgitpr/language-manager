#include "TaggerModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangCore
{

    class TaggerSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class TaggerCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(TaggerCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    TaggerSpec::~TaggerSpec() = default;


    TaggerSpec::TaggerSpec() : ModuleSpec(*new Impl(this->category())) {}

    TaggerCategory::~TaggerCategory() = default;

    std::string TaggerCategory::key() const { return "tagger"; }
    std::string TaggerCategory::category() const { return "tagger"; }

    TaggerCategory::TaggerCategory(PackageManager *env) : ModuleCategory(TaggerCategory::category(), env) {}

    static ModuleCategoryRegistrar<TaggerCategory> registrar;

} // namespace LangCore
