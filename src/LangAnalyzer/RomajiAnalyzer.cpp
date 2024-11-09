#include "RomajiAnalyzer.h"

#include <QRandomGenerator>

namespace LangMgr
{
    QSet<QString> romajiSet = {
        "cl",  "a",   "i",   "u",   "e",   "o",   "n",   "ka",  "ki",  "ku",  "ke",  "ko",  "kwa", "kwi", "kwu",
        "kwe", "kwo", "sa",  "si",  "su",  "se",  "so",  "sha", "shi", "shu", "she", "sho", "cha", "chi", "chu",
        "che", "cho", "tsa", "tsi", "tsu", "tse", "tso", "ta",  "ti",  "tu",  "te",  "to",  "na",  "ni",  "nu",
        "ne",  "no",  "ha",  "hi",  "hu",  "he",  "ho",  "fa",  "fi",  "fu",  "fe",  "fo",  "ma",  "mi",  "mu",
        "me",  "mo",  "ya",  "yi",  "yu",  "ye",  "yo",  "ra",  "ri",  "ru",  "re",  "ro",  "la",  "li",  "lu",
        "le",  "lo",  "wa",  "wi",  "wu",  "we",  "wo",  "ga",  "gi",  "gu",  "ge",  "go",  "gwa", "gwi", "gwu",
        "gwe", "gwo", "ja",  "ji",  "ju",  "je",  "jo",  "jya", "jyi", "jyu", "jye", "jyo", "za",  "zi",  "zu",
        "ze",  "zo",  "da",  "di",  "du",  "de",  "do",  "ba",  "bi",  "bu",  "be",  "bo",  "va",  "vi",  "vu",
        "ve",  "vo",  "pa",  "pi",  "pu",  "pe",  "po",  "kya", "kyi", "kyu", "kye", "kyo", "tya", "tyi", "tyu",
        "tye", "tyo", "dya", "dyi", "dyu", "dye", "dyo", "nya", "nyi", "nyu", "nye", "nyo", "hya", "hyi", "hyu",
        "hye", "hyo", "mya", "myi", "myu", "mye", "myo", "rya", "ryi", "ryu", "rye", "ryo", "gya", "gyi", "gyu",
        "gye", "gyo", "bya", "byi", "byu", "bye", "byo", "pya", "pyi", "pyu", "pye", "pyo"};

    static bool isLetter(const QChar &c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '\''; }

    bool RomajiAnalyzer::contains(const QString &input) const { return romajiSet.contains(input); }

    QList<LangNote> RomajiAnalyzer::split(const QString &input, const QString &g2pId) const {
        if (!enabled())
            return {LangNote(input)};
        QList<LangNote> result;

        int pos = 0;
        while (pos < input.length()) {
            const auto &currentChar = input[pos];
            LangNote note;
            if (isLetter(currentChar)) {
                const int start = pos;
                while (pos < input.length() && isLetter(input[pos])) {
                    pos++;
                }

                note.lyric = input.mid(start, pos - start);
                if (contains(note.lyric)) {
                    note.g2pId = g2pId;
                    if (discardResult())
                        continue;
                } else {
                    note.g2pId = QStringLiteral("unknown");
                }
            } else {
                const int start = pos;
                while (pos < input.length() && !isLetter(input[pos])) {
                    pos++;
                }
                note.lyric = input.mid(start, pos - start);
                note.g2pId = QStringLiteral("unknown");
            }
            if (!note.lyric.isEmpty())
                result.append(note);
        }
        return result;
    }

    template <typename T>
    static T getRandomElementFromSet(const QSet<T> &set) {
        const int size = set.size();

        int randomIndex = QRandomGenerator::global()->bounded(size);

        auto it = set.constBegin();
        std::advance(it, randomIndex);

        return *it;
    }

    QString RomajiAnalyzer::randString() const { return getRandomElementFromSet(romajiSet); }

} // namespace LangMgr
