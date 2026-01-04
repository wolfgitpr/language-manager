#ifndef LANGMGR_G2PPROVIDER_H
#define LANGMGR_G2PPROVIDER_H

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Tool/G2pContrib.h>

namespace LangMgr
{

    class G2pProvider : public NamedObject {
    public:
        /// The highest g2p API version currently supported by this model.
        virtual int apiLevel() const = 0;

        /// Called when \c G2pSpec loads.
        virtual Expected<NO<G2pConfiguration>> createConfiguration(const G2pSpec *spec) const = 0;
    };

} // namespace LangMgr

#endif // LANGMGR_G2PPROVIDER_H
