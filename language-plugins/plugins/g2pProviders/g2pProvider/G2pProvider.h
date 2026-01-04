#ifndef LANGPLG_G2PPROVIDER_H
#define LANGPLG_G2PPROVIDER_H

#include <LangMgr/Tool/G2pProvider.h>

namespace LangPlugins
{

    class G2pProvider : public LangMgr::G2pProvider {
    public:
        G2pProvider();
        ~G2pProvider() override;

    public:
        int apiLevel() const override;

        LangMgr::Expected<LangMgr::NO<LangMgr::G2pConfiguration>>
        createConfiguration(const LangMgr::G2pSpec *spec) const override;
    };

} // namespace LangPlugins

#endif // LANGPLG_G2PPROVIDER_H
