#include "PunctuationAnalyzer.h"

#include <qrandom.h>

namespace LangMgr {
    bool PunctuationAnalyzer::contains(const QChar &c) const {
        return c.isPunct() && c != '-';
    }

    QString PunctuationAnalyzer::randString() const {
        QChar randomPunctuation;
        do {
            const int codePoint = QRandomGenerator::global()->bounded(0x2000, 0x206F + 1);
            randomPunctuation = QChar(codePoint);
        } while (randomPunctuation == QChar('-'));

        return randomPunctuation;
    }

} // LangMgr