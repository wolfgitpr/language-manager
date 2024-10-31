#include "SpaceG2p.h"

namespace LangMgr
{
    SpaceG2p::SpaceG2p(const QString &id, const QString &categroy, QObject *parent) : UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Space"));
        setDescription(tr("Space, no conversion required."));
    }
} // namespace LangMgr
