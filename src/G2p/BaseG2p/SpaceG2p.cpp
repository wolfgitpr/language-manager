#include "SpaceG2p.h"

#include "LangAnalysis/BaseAnalysis/SpaceAnalysis.h"

namespace LangMgr
{
    SpaceG2p::SpaceG2p(const QString &id, QObject *parent) : UnknownG2p(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Space"));
        setDescription(tr("Space, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("space", new SpaceAnalysis());
    }
} // namespace LangMgr
