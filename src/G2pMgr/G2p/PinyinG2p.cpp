#include "PinyinG2p.h"

namespace LangMgr
{
    PinyinG2p::PinyinG2p(const QString &id, const QString &categroy, QObject *parent) :
        EnglishG2p(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Pinyin"));
        setDescription(tr("Pinyin, no conversion required."));
    }
} // namespace LangMgr
