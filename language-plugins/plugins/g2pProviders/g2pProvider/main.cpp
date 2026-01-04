#include <LangMgr/Tool/G2pProviderPlugin.h>
#include "G2pProvider.h"

namespace LangPlugins
{

    class G2pProviderPlugin : public LangMgr::G2pProviderPlugin {
    public:
        G2pProviderPlugin() = default;

        const char *key() const override { return "g2p"; }

        LangMgr::NO<LangMgr::G2pProvider> create() override { return LangMgr::NO<G2pProvider>::create(); }
    };

} // namespace LangPlugins

LANGMGR_EXPORT_PLUGIN(LangPlugins::G2pProviderPlugin)
