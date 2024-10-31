#include "NumberG2p.h"

namespace LangMgr
{
    NumberG2p::NumberG2p(const QString &id, const QString &categroy, QObject *parent) :
        UnknownG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Number"));
        setDescription(tr("Number, no conversion required."));
    }
} // namespace LangMgr
