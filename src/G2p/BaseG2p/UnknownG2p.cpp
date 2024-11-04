#include "UnknownG2p.h"

#include "LangAnalysis/BaseAnalysis/UnknownAnalysis.h"

namespace LangMgr
{
    UnknownG2p::UnknownG2p(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Unknown"));
        setDescription(tr("Unknown language, no conversion required."));
        m_langFactory.insert("unknown", new UnknownAnalysis());
    }

    UnknownG2p::~UnknownG2p() = default;

    QList<LangNote> UnknownG2p::convert(const QStringList &input) const {
        QList<LangNote> result;
        for (const auto &i : input) {
            LangNote langNote;
            langNote.lyric = i;
            langNote.syllable = i;
            langNote.candidates = QStringList() << i;
            result.append(langNote);
        }
        return result;
    }
} // namespace LangMgr
