#include "UnknownAnalyzer.h"

#include <qrandom.h>

namespace LangMgr
{
    bool UnknownAnalyzer::contains(const QString &input) const { return true; }

    QList<LangNote> UnknownAnalyzer::split(const QString &input, const QString &g2pId) const {
        if (discardResult())
            return {};
        return {LangNote(input)};
    }

    QString UnknownAnalyzer::randString() const {
        const int unicode = QRandomGenerator::global()->bounded(0x2200, 0x22ff + 1);
        return {QChar(unicode)};
    }

} // namespace LangMgr
