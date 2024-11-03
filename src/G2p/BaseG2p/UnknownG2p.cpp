#include "UnknownG2p.h"

namespace LangMgr
{
    UnknownG2p::UnknownG2p(const QString &id, const QString &categroy, QObject *parent) : IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Unknown"));
        setDescription(tr("Unknown language, no conversion required."));
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

    QJsonObject UnknownG2p::config() { return {}; }
} // namespace LangMgr
