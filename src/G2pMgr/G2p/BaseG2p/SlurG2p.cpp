#include "SlurG2p.h"

namespace LangMgr
{
    SlurG2p::SlurG2p(const QString &id, const QString &categroy, QObject *parent) : UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Slur"));
        setDescription(tr("Slur, no conversion required."));
    }
} // namespace LangMgr