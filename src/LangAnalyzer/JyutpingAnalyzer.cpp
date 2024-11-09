#include "JyutpingAnalyzer.h"

#include <QRandomGenerator>

namespace LangMgr
{
    QSet<QString> jyutpingSet = {
        "aa",    "aai",   "aau",   "aam",   "aan",    "aang",  "aap",   "aat",   "aak",    "ai",    "au",     "am",
        "ang",   "ap",    "ak",    "e",     "ei",     "o",     "oi",    "ou",    "on",     "ong",   "ok",     "ung",
        "uk",    "baa",   "baai",  "baau",  "baan",   "baang", "baat",  "baak",  "bai",    "bau",   "bam",    "ban",
        "bang",  "bat",   "bak",   "be",    "bei",    "beng",  "bek",   "biu",   "bin",    "bing",  "bit",    "bik",
        "bo",    "bou",   "bong",  "bok",   "bui",    "bun",   "bung",  "but",   "buk",    "paa",   "paai",   "paau",
        "paan",  "paang", "paak",  "pai",   "pau",    "pan",   "pang",  "pat",   "pei",    "peng",  "pek",    "piu",
        "pin",   "ping",  "pit",   "pik",   "po",     "poi",   "pou",   "pong",  "pok",    "pui",   "pun",    "pung",
        "put",   "puk",   "maa",   "maai",  "maau",   "maan",  "maang", "maat",  "maak",   "mai",   "mau",    "man",
        "mang",  "mat",   "mak",   "me",    "mei",    "meng",  "mi",    "miu",   "min",    "ming",  "mit",    "mik",
        "mo",    "mou",   "mong",  "mok",   "mui",    "mun",   "mung",  "mut",   "muk",    "faa",   "faai",   "faan",
        "faat",  "faak",  "fai",   "fau",   "fan",    "fang",  "fat",   "fe",    "fei",    "fiu",   "fo",     "fong",
        "fok",   "fu",    "fui",   "fun",   "fung",   "fut",   "fuk",   "daa",   "daai",   "daam",  "daan",   "daap",
        "daat",  "daak",  "dai",   "dau",   "dam",    "dan",   "dang",  "dap",   "dat",    "dak",   "de",     "dei",
        "deu",   "deng",  "dek",   "di",    "diu",    "dim",   "din",   "ding",  "dip",    "dit",   "dik",    "do",
        "doi",   "dou",   "dong",  "dok",   "doe",    "doek",  "deoi",  "deon",  "deot",   "dung",  "duk",    "dyun",
        "dyut",  "taa",   "taai",  "taam",  "taan",   "taap",  "taat",  "tai",   "tau",    "tam",   "tan",    "tang",
        "teng",  "tek",   "tiu",   "tim",   "tin",    "ting",  "tip",   "tit",   "tik",    "to",    "toi",    "tou",
        "tong",  "tok",   "toe",   "teoi",  "teon",   "tung",  "tuk",   "tyun",  "tyut",   "naa",   "naai",   "naau",
        "naam",  "naan",  "naap",  "naat",  "nai",    "nau",   "nam",   "nan",   "nang",   "nap",   "nat",    "nak",
        "ne",    "nei",   "ni",    "niu",   "nim",    "nin",   "ning",  "nip",   "nik",    "no",    "noi",    "nou",
        "nong",  "nok",   "noeng", "neoi",  "neot",   "nung",  "nuk",   "nyun",  "laa",    "laai",  "laau",   "laam",
        "laan",  "laang", "laap",  "laat",  "laak",   "lai",   "lau",   "lam",   "lang",   "lap",   "lat",    "lak",
        "le",    "lei",   "lem",   "leng",  "lek",    "li",    "liu",   "lim",   "lin",    "ling",  "lip",    "lit",
        "lik",   "lo",    "loi",   "lou",   "long",   "lok",   "loeng", "loek",  "leoi",   "leon",  "leot",   "lung",
        "luk",   "lyun",  "lyut",  "gaa",   "gaai",   "gaau",  "gaam",  "gaan",  "gaang",  "gaap",  "gaat",   "gaak",
        "gai",   "gau",   "gam",   "gan",   "gang",   "gap",   "gat",   "ge",    "gei",    "geng",  "gep",    "giu",
        "gim",   "gin",   "ging",  "gip",   "git",    "gik",   "go",    "goi",   "gou",    "gon",   "gong",   "got",
        "gok",   "goe",   "goeng", "goek",  "geoi",   "gu",    "gun",   "gung",  "guk",    "gyun",  "gyut",   "kaa",
        "kaai",  "kaau",  "kaat",  "kaak",  "kai",    "kau",   "kam",   "kan",   "kang",   "kap",   "kat",    "ke",
        "kei",   "kek",   "kiu",   "kim",   "kin",    "king",  "kit",   "kik",   "ko",     "koi",   "kong",   "kok",
        "koe",   "koeng", "koek",  "keoi",  "ku",     "kui",   "kung",  "kut",   "kuk",    "kyun",  "kyut",   "ngaa",
        "ngaai", "ngaau", "ngaam", "ngaan", "ngaang", "ngaap", "ngaat", "ngaak", "ngai",   "ngau",  "ngam",   "ngan",
        "ngang", "ngap",  "ngat",  "ngak",  "ngit",   "ngo",   "ngoi",  "ngou",  "ngon",   "ngong", "ngok",   "ngung",
        "nguk",  "haa",   "haai",  "haau",  "haam",   "haan",  "haang", "haap",  "haak",   "hai",   "hau",    "ham",
        "han",   "hang",  "hap",   "hat",   "hak",    "hei",   "heng",  "hek",   "hiu",    "him",   "hin",    "hing",
        "hip",   "hit",   "hik",   "ho",    "hoi",    "hou",   "hon",   "hong",  "hot",    "hok",   "hoe",    "hoeng",
        "heoi",  "hung",  "huk",   "hyun",  "hyut",   "gwaa",  "gwaai", "gwaan", "gwaang", "gwaat", "gwaak",  "gwai",
        "gwan",  "gwang", "gwat",  "gwing", "gwik",   "gwo",   "gwong", "gwok",  "kwaa",   "kwaai", "kwaang", "kwai",
        "kwan",  "kwik",  "kwong", "kwok",  "waa",    "waai",  "waan",  "waang", "waat",   "waak",  "wai",    "wan",
        "wang",  "wat",   "wing",  "wik",   "wo",     "wong",  "wok",   "wu",    "wui",    "wun",   "wut",    "zaa",
        "zaai",  "zaau",  "zaam",  "zaan",  "zaang",  "zaap",  "zaat",  "zaak",  "zai",    "zau",   "zam",    "zan",
        "zang",  "zap",   "zat",   "zak",   "ze",     "zeng",  "zek",   "zi",    "ziu",    "zim",   "zin",    "zing",
        "zip",   "zit",   "zik",   "zo",    "zoi",    "zou",   "zong",  "zok",   "zoeng",  "zoek",  "zeoi",   "zeon",
        "zeot",  "zung",  "zuk",   "zyu",   "zyun",   "zyut",  "caa",   "caai",  "caau",   "caam",  "caan",   "caang",
        "caap",  "caat",  "caak",  "cai",   "cau",    "cam",   "can",   "cang",  "cap",    "cat",   "cak",    "ce",
        "ceng",  "cek",   "ci",    "ciu",   "cim",    "cin",   "cing",  "cip",   "cit",    "cik",   "co",     "coi",
        "cou",   "cong",  "cok",   "coeng", "coek",   "ceoi",  "ceon",  "ceot",  "cun",    "cung",  "cuk",    "cyu",
        "cyun",  "cyut",  "saa",   "saai",  "saau",   "saam",  "saan",  "saang", "saap",   "saat",  "saak",   "sai",
        "sau",   "sam",   "san",   "sang",  "sap",    "sat",   "sak",   "se",    "sei",    "seng",  "sek",    "si",
        "siu",   "sim",   "sin",   "sing",  "sip",    "sit",   "sik",   "so",    "soi",    "sou",   "song",   "sok",
        "soeng", "soek",  "seoi",  "seon",  "seot",   "sung",  "suk",   "syu",   "syun",   "syut",  "jaa",    "jaai",
        "jaak",  "jai",   "jau",   "jam",   "jan",    "jap",   "jat",   "je",    "jeng",   "ji",    "jiu",    "jim",
        "jin",   "jing",  "jip",   "jit",   "jik",    "jo",    "joeng", "joek",  "jeoi",   "jeon",  "jung",   "juk",
        "jyu",   "jyun",  "jyut"};

    static bool isLetter(const QChar &c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '\''; }

    bool JyutpingAnalyzer::contains(const QString &input) const { return jyutpingSet.contains(input); }

    QList<LangNote> JyutpingAnalyzer::split(const QString &input, const QString &g2pId) const {
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

    QString JyutpingAnalyzer::randString() const { return getRandomElementFromSet(jyutpingSet); }

} // namespace LangMgr
