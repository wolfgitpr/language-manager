#include "JyutpingG2p.h"

namespace LangMgr
{
    JyutpingG2p::JyutpingG2p(const QString &id, const QString &categroy, QObject *parent) : EnglishG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Jyutping"));
        setDescription(tr("Jyutping, no conversion required."));
    }
} // namespace LangMgr
