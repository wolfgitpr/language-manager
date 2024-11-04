#include "SlurG2p.h"

#include "LangAnalysis/BaseAnalysis/SlurAnalysis.h"

namespace LangMgr
{
    SlurG2p::SlurG2p(const QString &id, QObject *parent) : UnknownG2p(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Slur"));
        setDescription(tr("Slur, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("slur", new SlurAnalysis());
    }
} // namespace LangMgr
