#ifndef LANGMGR_G2PPROVIDERPLUGIN_H
#define LANGMGR_G2PPROVIDERPLUGIN_H

#include <LangMgr/Plugin/Plugin.h>
#include <LangMgr/Tool/G2pProvider.h>

namespace LangMgr
{

    class G2pProviderPlugin : public Plugin {
    public:
        G2pProviderPlugin() = default;
        ~G2pProviderPlugin() override;

        const char *iid() const override { return "org.openvpi.G2pProvider"; }

    public:
        virtual NO<G2pProvider> create() = 0;

    public:
        STDCORELIB_DISABLE_COPY(G2pProviderPlugin)
    };
    inline G2pProviderPlugin::~G2pProviderPlugin() {}

} // namespace LangMgr

#endif // LANGMGR_G2PPROVIDERPLUGIN_H
