#ifndef LANGPLG_API_G2PPROVIDERAPIL1_H
#define LANGPLG_API_G2PPROVIDERAPIL1_H

#include <LangMgr/Tool/G2pContrib.h>

namespace LangPlugins::Api::G2pProvider::L1
{

    inline constexpr char API_NAME[] = "g2p";

    inline constexpr int API_LEVEL = 1;

    class G2pProviderConfiguration : public LangMgr::G2pConfiguration {
    public:
        G2pProviderConfiguration() : G2pConfiguration(API_NAME, API_LEVEL) {}

        std::filesystem::path dict;
    };

} // namespace LangPlugins::Api::G2pProvider::L1

#endif // LANGPLG_API_G2PPROVIDERAPIL1_H
