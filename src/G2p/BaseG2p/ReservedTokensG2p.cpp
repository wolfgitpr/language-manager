#include "ReservedTokensG2p.h"

namespace LangMgr
{
    ReservedTokensG2p::ReservedTokensG2p(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("ReservedTokens"));
        setDescription(tr("ReservedTokens, no conversion required."));
    }

    ReservedTokensG2p::~ReservedTokensG2p() = default;
} // namespace LangMgr
