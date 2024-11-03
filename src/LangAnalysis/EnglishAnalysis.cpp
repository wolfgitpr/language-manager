#include "EnglishAnalysis.h"

#include <qrandom.h>

namespace LangMgr {
    bool isLetter(const QChar &c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '\'';
    }

    bool EnglishAnalysis::contains(const QChar &c) const {
        return isLetter(c);
    }

    bool EnglishAnalysis::contains(const QString &input) const {
        for (const QChar &ch : input) {
            if (!isLetter(ch)) {
                return false;
            }
        }
        return true;
    }

    QString EnglishAnalysis::randString() const {
        QString word;
        const QString alphabet = "abcdefghijklmnopqrstuvwxyz";
        const int length = QRandomGenerator::global()->bounded(1,12);

        for (int i = 0; i < length; ++i) {
            const int index = static_cast<int>(QRandomGenerator::global()->bounded(alphabet.length()));
            word.append(alphabet.at(index));
        }

        return word;
    }

} // LangMgr