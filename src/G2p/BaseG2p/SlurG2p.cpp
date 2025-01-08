#include "SlurG2p.h"

#include "LangAnalyzer/BaseAnalyzer/SlurAnalyzer.h"

namespace LangMgr
{
    SlurG2p::SlurG2p(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Slur"));
        setDescription(tr("Slur, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("slur", new SlurAnalyzer());
    }
} // namespace LangMgr
