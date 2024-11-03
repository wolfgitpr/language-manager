#include "RomajiG2p.h"

namespace LangMgr
{
    RomajiG2p::RomajiG2p(const QString &id, const QString &categroy, QObject *parent) : EnglishG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Romaji"));
        setDescription(tr("Romaji, no conversion required."));
    }
} // namespace LangMgr
