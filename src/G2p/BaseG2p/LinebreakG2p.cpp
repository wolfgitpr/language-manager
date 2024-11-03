#include "LinebreakG2p.h"

namespace LangMgr
{
    LinebreakG2p::LinebreakG2p(const QString &id, const QString &categroy, QObject *parent) :
        UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Linebreak"));
        setDescription(tr("Linebreak, no conversion required."));
    }
} // namespace LangMgr
