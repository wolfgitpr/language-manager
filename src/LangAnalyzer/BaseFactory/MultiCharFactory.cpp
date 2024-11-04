#include "MultiCharFactory.h"

namespace LangMgr
{
    QList<LangNote> MultiCharFactory::split(const QString &input, const QString &g2pId) const {
        if (!enabled())
            return {LangNote(input)};
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
                note.g2pId = g2pId;
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
                result.append(note);
        }
        return result;
    }
} // namespace LangMgr
