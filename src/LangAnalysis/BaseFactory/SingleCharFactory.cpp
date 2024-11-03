#include "SingleCharFactory.h"
#include <QDebug>

namespace LangMgr
{
    bool SingleCharFactory::contains(const QChar &c) const {
        Q_UNUSED(c);
        return false;
    }

    bool SingleCharFactory::contains(const QString &input) const {
        if (input.size() == 1)
            return contains(input.at(0));
        return false;
    }

    QList<LangNote> SingleCharFactory::split(const QString &input, const QString &g2pId) const {
        if (!enabled())
            return {LangNote(input)};
        QList<LangNote> result;

        int pos = 0;
        while (pos < input.length()) {
            const QChar &currentChar = input[pos];
            LangNote note;

            if (contains(currentChar)) {
                note.lyric = input.mid(pos, 1);
                note.g2pId = g2pId;
                pos++;
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
