#include "NumberAnalyzer.h"

#include <qrandom.h>

namespace LangMgr {
    bool isNumber(const QChar &c) {
        return c.isDigit();
    }

    bool NumberAnalyzer::contains(const QChar &c) const {
        return isNumber(c);
    }

    bool NumberAnalyzer::contains(const QString &input) const {
        for (const QChar &ch : input) {
            if (!isNumber(ch)) {
                return false;
            }
        }
        return true;
    }

    QString NumberAnalyzer::randString() const {
        QString word;
        const QString alphabet = "0123456789";
        const int length = QRandomGenerator::global()->bounded(1, 5);

        for (int i = 0; i < length; ++i) {
            const int index =
                static_cast<int>(QRandomGenerator::global()->bounded(alphabet.length()));
            word.append(alphabet.at(index));
        }

        return word;
    }

} // LangMgr