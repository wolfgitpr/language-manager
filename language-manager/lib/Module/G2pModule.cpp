#include "G2pModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class G2pDefinition::Impl : public ModuleDefinition::Impl {
    public:
        Impl(const std::string &category) : ModuleDefinition::Impl(category) {}
    };

    class G2pCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(G2pCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    G2pDefinition::~G2pDefinition() = default;


    G2pDefinition::G2pDefinition() : ModuleDefinition(*new Impl(this->category())) {}

    G2pCategory::~G2pCategory() = default;

    std::string G2pCategory::key() const { return "g2p"; }
    std::string G2pCategory::category() const { return "g2p"; }

    G2pCategory::G2pCategory(PackageManager *env) : ModuleCategory(G2pCategory::category(), env) {}

    ModuleCategoryRegistrar<G2pCategory> registrar;

} // namespace LangMgr
