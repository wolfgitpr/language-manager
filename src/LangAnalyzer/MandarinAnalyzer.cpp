#include "MandarinAnalyzer.h"

#include <qrandom.h>

namespace LangMgr {
    bool isHanzi(const QChar &c) {
        return c >= QChar(0x4e00) && c <= QChar(0x9fa5);
    }

    bool MandarinAnalyzer::contains(const QChar &c) const {
        return isHanzi(c);
    }

    QString MandarinAnalyzer::randString() const {
        const int unicode = QRandomGenerator::global()->bounded(0x4e00, 0x9fa5 + 1);
        return {QChar(unicode)};
    }

} // LangMgr