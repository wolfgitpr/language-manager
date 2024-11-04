#include "PunctuationG2p.h"

#include "LangAnalyzer/BaseAnalyzer/PunctuationAnalyzer.h"

namespace LangMgr
{
    PunctuationG2p::PunctuationG2p(const QString &id, QObject *parent) : UnknownG2p(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Punctuation"));
        setDescription(tr("Punctuation, no conversion required."));
        m_langFactory.clear();
        m_langFactory.insert("punctuation", new PunctuationAnalyzer());
    }
} // namespace LangMgr
