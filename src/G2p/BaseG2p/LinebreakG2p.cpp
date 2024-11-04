#include "LinebreakG2p.h"

#include "LangAnalyzer/BaseAnalyzer/LinebreakAnalyzer.h"

namespace LangMgr
{
    LinebreakG2p::LinebreakG2p(const QString &id, QObject *parent) : UnknownG2p(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Linebreak"));
        setDescription(tr("Linebreak, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("linebreak", new LinebreakAnalyzer());
    }
} // namespace LangMgr
