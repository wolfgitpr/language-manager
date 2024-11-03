#include "PunctuationG2p.h"

#include "LangAnalysis/BaseAnalysis/PunctuationAnalysis.h"

namespace LangMgr
{
    PunctuationG2p::PunctuationG2p(const QString &id, const QString &categroy, QObject *parent) :
        UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Punctuation"));
        setDescription(tr("Punctuation, no conversion required."));
        m_langFactory.insert("punctuation", new PunctuationAnalysis());
    }
} // namespace LangMgr
