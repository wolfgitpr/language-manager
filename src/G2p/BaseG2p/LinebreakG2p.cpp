#include "LinebreakG2p.h"

#include "LangAnalysis/BaseAnalysis/LinebreakAnalysis.h"

namespace LangMgr
{
    LinebreakG2p::LinebreakG2p(const QString &id, const QString &categroy, QObject *parent) :
        UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Linebreak"));
        setDescription(tr("Linebreak, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("linebreak", new LinebreakAnalysis());
    }
} // namespace LangMgr
