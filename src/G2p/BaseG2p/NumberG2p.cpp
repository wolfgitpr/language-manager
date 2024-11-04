#include "NumberG2p.h"

#include "LangAnalysis/BaseAnalysis/NumberAnalysis.h"

namespace LangMgr
{
    NumberG2p::NumberG2p(const QString &id, QObject *parent) : UnknownG2p(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Number"));
        setDescription(tr("Number, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("number", new NumberAnalysis());
    }
} // namespace LangMgr
