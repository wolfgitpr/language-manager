#include "TaggerModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class TaggerDefinition::Impl : public ModuleDefinition::Impl {
    public:
        Impl(const std::string &category) : ModuleDefinition::Impl(category) {}
    };

    class TaggerCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(TaggerCategory *decl, const std::string &category, Manager *su) :
            ModuleCategory::Impl(decl, category, su) {}

        ~Impl() override = default;
    };

    TaggerDefinition::~TaggerDefinition() = default;


    TaggerDefinition::TaggerDefinition() : ModuleDefinition(*new Impl(this->category())) {}

    TaggerCategory::~TaggerCategory() = default;

    std::string TaggerCategory::key() const { return "tagger"; }
    std::string TaggerCategory::category() const { return "tagger"; }

    TaggerCategory::TaggerCategory(PackageManager *env) : ModuleCategory(TaggerCategory::category(), env) {}

    ModuleCategoryRegistrar<TaggerCategory> registrar;

} // namespace LangMgr
