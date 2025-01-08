#include "ReservedTokensG2p.h"

namespace LangMgr
{
    ReservedTokensG2p::ReservedTokensG2p(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("ReservedTokens"));
        setDescription(tr("ReservedTokens, no conversion required."));
    }

    ReservedTokensG2p::~ReservedTokensG2p() = default;

    QList<LangNote> ReservedTokensG2p::convert(const QStringList &input) const {
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
