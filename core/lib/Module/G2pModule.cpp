#include "G2pModule.h"

#include <fstream>

#include <stdcorelib/path.h>

#include "Module_p.h"

namespace fs = std::filesystem;

namespace LangCore
{

    class G2pSpec::Impl : public ModuleSpec::Impl {
    public:
        Impl(const std::string &category) : ModuleSpec::Impl(category) {}
    };

    class G2pCategory::Impl : public ModuleCategory::Impl {
    public:
        explicit Impl(G2pCategory *decl, const std::string &category, PackageManager *mgr) :
            ModuleCategory::Impl(decl, category, mgr) {}

        ~Impl() override = default;
    };

    G2pSpec::~G2pSpec() = default;

    G2pSpec::G2pSpec() : ModuleSpec(*new Impl(this->category())) {}

    G2pCategory::~G2pCategory() = default;

    std::string G2pCategory::key() const { return "g2p"; }
    std::string G2pCategory::category() const { return "g2p"; }

    G2pCategory::G2pCategory(PackageManager *env) : ModuleCategory(G2pCategory::category(), env) {}

    static ModuleCategoryRegistrar<G2pCategory> registrar;

} // namespace LangCore
