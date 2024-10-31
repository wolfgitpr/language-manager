#include "Romaji.h"

namespace LangMgr
{
    Romaji::Romaji(const QString &id, const QString &categroy, QObject *parent) : EnglishG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Romaji"));
        setDescription(tr("Romaji, no conversion required."));
    }
} // namespace LangMgr
