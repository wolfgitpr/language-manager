#include "TaggerModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class TaggerSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class TaggerCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(TaggerCategory *decl, const std::string &category, Manager *su) :
            ModuleCategory::Impl(decl, category, su) {}

        ~Impl() override = default;
    };

    TaggerSpec::~TaggerSpec() = default;


    TaggerSpec::TaggerSpec() : ModuleSpec(*new Impl(this->category())) {}

    TaggerCategory::~TaggerCategory() = default;

    std::string TaggerCategory::key() const { return "tagger"; }
    std::string TaggerCategory::category() const { return "tagger"; }

    TaggerCategory::TaggerCategory(PackageManager *env) : ModuleCategory(TaggerCategory::category(), env) {}

    ModuleCategoryRegistrar<TaggerCategory> registrar;

} // namespace LangMgr
