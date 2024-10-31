#include "Jyutping.h"

namespace LangMgr
{
    Jyutping::Jyutping(const QString &id, const QString &categroy, QObject *parent) : EnglishG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Jyutping"));
        setDescription(tr("Jyutping, no conversion required."));
    }
} // namespace LangMgr
