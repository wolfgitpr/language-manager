#include "KanaAnalysis.h"

#include <qrandom.h>

namespace LangMgr
{
    static QStringView specialKana = QStringLiteral("ャュョゃゅょァィゥェォぁぃぅぇぉ");

    bool isKana(const QChar &c) {
        return ((c >= QChar(0x3040) && c <= QChar(0x309F)) || (c >= QChar(0x30A0) && c <= QChar(0x30FF)));
    }

    bool isSpecialKana(const QChar &c) { return specialKana.contains(c); }

    bool KanaAnalysis::contains(const QString &input) const {
        for (const QChar &ch : input) {
            if (!(isKana(ch) && !isSpecialKana(ch))) {
                return false;
            }
        }
        return true;
    }

    QList<LangNote> KanaAnalysis::split(const QString &input, const QString &g2pId) const {
        if (!enabled())
            return {LangNote(input)};
        QList<LangNote> results;

        int pos = 0;
        while (pos < input.length()) {
            const auto &currentChar = input[pos];
            LangNote note;
            if (isKana(currentChar)) {
                const int length = pos + 1 < input.length() && isSpecialKana(input[pos + 1]) ? 2 : 1;
                note.lyric = input.mid(pos, length);
                note.g2pId = g2pId;
                pos += length;
                if (discardResult())
                    continue;
            } else {
                const int start = pos;
                while (pos < input.length() && !contains(input[pos])) {
                    pos++;
                }
                note.lyric = input.mid(start, pos - start);
                note.g2pId = "unknown";
            }
            if (!note.lyric.isEmpty())
                results.append(note);
        }

        return results;
    }

    QString KanaAnalysis::randString() const {
        const int unicode = QRandomGenerator::global()->bounded(0x3040, 0x309F + 1);
        const int unicode2 = QRandomGenerator::global()->bounded(0x30A0, 0x30FF + 1);

        QString result = QRandomGenerator::global()->bounded(2) == 0 ? QChar(unicode) : QChar(unicode2);
        if (QRandomGenerator::global()->bounded(2) == 0) {
            result += specialKana[QRandomGenerator::global()->bounded(static_cast<quint32>(specialKana.size()))];
        }
        return result;
    }


} // LangMgr