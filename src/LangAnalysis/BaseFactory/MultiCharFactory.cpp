#include "MultiCharFactory.h"

namespace LangMgr
{
    QList<LangNote> MultiCharFactory::split(const QString &input, const QString &g2pId) const {
        QList<LangNote> result;

        int pos = 0;
        while (pos < input.length()) {
            const auto &currentChar = input[pos];
            LangNote note;
            if (contains(currentChar)) {
                const int start = pos;
                while (pos < input.length() && contains(input[pos])) {
                    pos++;
                }
                note.lyric = input.mid(start, pos - start);
                note.language = id();
                note.category = category();
                note.g2pId = g2pId;
            } else {
                const int start = pos;
                while (pos < input.length() && !contains(input[pos])) {
                    pos++;
                }
                note.lyric = input.mid(start, pos - start);
                note.language = "unknown";
                note.category = "unknown";
                note.g2pId = "unknown";
            }
            if (!note.lyric.isEmpty())
                result.append(note);
        }
        return result;
    }
} // namespace LangMgr
