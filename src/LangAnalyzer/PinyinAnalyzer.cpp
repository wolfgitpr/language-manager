#include "PinyinAnalyzer.h"

#include <QRandomGenerator>

namespace LangMgr
{
    static QSet<QString> pinyinSet = {
        "a",     "ai",    "an",     "ang",   "ao",     "ba",    "bai",    "ban",   "bang",  "bao",   "be",    "bei",
        "ben",   "beng",  "ber",    "bi",    "bia",    "bian",  "biang",  "biao",  "bie",   "bin",   "bing",  "biong",
        "biu",   "bo",    "bong",   "bou",   "bu",     "bua",   "buai",   "buan",  "buang", "bui",   "bun",   "bv",
        "bve",   "ca",    "cai",    "can",   "cang",   "cao",   "ce",     "cei",   "cen",   "ceng",  "cer",   "cha",
        "chai",  "chan",  "chang",  "chao",  "che",    "chei",  "chen",   "cheng", "cher",  "chi",   "chong", "chou",
        "chu",   "chua",  "chuai",  "chuan", "chuang", "chui",  "chun",   "chuo",  "chv",   "chyi",  "ci",    "cong",
        "cou",   "cu",    "cua",    "cuai",  "cuan",   "cuang", "cui",    "cun",   "cuo",   "cv",    "cyi",   "da",
        "dai",   "dan",   "dang",   "dao",   "de",     "dei",   "den",    "deng",  "der",   "di",    "dia",   "dian",
        "diang", "diao",  "die",    "din",   "ding",   "diong", "diu",    "dong",  "dou",   "du",    "dua",   "duai",
        "duan",  "duang", "dui",    "dun",   "duo",    "dv",    "dve",    "e",     "ei",    "en",    "eng",   "er",
        "fa",    "fai",   "fan",    "fang",  "fao",    "fe",    "fei",    "fen",   "feng",  "fer",   "fi",    "fia",
        "fian",  "fiang", "fiao",   "fie",   "fin",    "fing",  "fiong",  "fiu",   "fo",    "fong",  "fou",   "fu",
        "fua",   "fuai",  "fuan",   "fuang", "fui",    "fun",   "fv",     "fve",   "ga",    "gai",   "gan",   "gang",
        "gao",   "ge",    "gei",    "gen",   "geng",   "ger",   "gi",     "gia",   "gian",  "giang", "giao",  "gie",
        "gin",   "ging",  "giong",  "giu",   "gong",   "gou",   "gu",     "gua",   "guai",  "guan",  "guang", "gui",
        "gun",   "guo",   "gv",     "gve",   "ha",     "hai",   "han",    "hang",  "hao",   "he",    "hei",   "hen",
        "heng",  "her",   "hi",     "hia",   "hian",   "hiang", "hiao",   "hie",   "hin",   "hing",  "hiong", "hiu",
        "hong",  "hou",   "hu",     "hua",   "huai",   "huan",  "huang",  "hui",   "hun",   "huo",   "hv",    "hve",
        "ji",    "jia",   "jian",   "jiang", "jiao",   "jie",   "jin",    "jing",  "jiong", "jiu",   "ju",    "juan",
        "jue",   "jun",   "ka",     "kai",   "kan",    "kang",  "kao",    "ke",    "kei",   "ken",   "keng",  "ker",
        "ki",    "kia",   "kian",   "kiang", "kiao",   "kie",   "kin",    "king",  "kiong", "kiu",   "kong",  "kou",
        "ku",    "kua",   "kuai",   "kuan",  "kuang",  "kui",   "kun",    "kuo",   "kv",    "kve",   "la",    "lai",
        "lan",   "lang",  "lao",    "le",    "lei",    "len",   "leng",   "ler",   "li",    "lia",   "lian",  "liang",
        "liao",  "lie",   "lin",    "ling",  "liong",  "liu",   "lo",     "long",  "lou",   "lu",    "lua",   "luai",
        "luan",  "luang", "lui",    "lun",   "luo",    "lv",    "lve",    "ma",    "mai",   "man",   "mang",  "mao",
        "me",    "mei",   "men",    "meng",  "mer",    "mi",    "mia",    "mian",  "miang", "miao",  "mie",   "min",
        "ming",  "miong", "miu",    "mo",    "mong",   "mou",   "mu",     "mua",   "muai",  "muan",  "muang", "mui",
        "mun",   "mv",    "mve",    "na",    "nai",    "nan",   "nang",   "nao",   "ne",    "nei",   "nen",   "neng",
        "ner",   "ni",    "nia",    "nian",  "niang",  "niao",  "nie",    "nin",   "ning",  "niong", "niu",   "nong",
        "nou",   "nu",    "nua",    "nuai",  "nuan",   "nuang", "nui",    "nun",   "nuo",   "nv",    "nve",   "o",
        "ong",   "ou",    "pa",     "pai",   "pan",    "pang",  "pao",    "pe",    "pei",   "pen",   "peng",  "per",
        "pi",    "pia",   "pian",   "piang", "piao",   "pie",   "pin",    "ping",  "piong", "piu",   "po",    "pong",
        "pou",   "pu",    "pua",    "puai",  "puan",   "puang", "pui",    "pun",   "pv",    "pve",   "qi",    "qia",
        "qian",  "qiang", "qiao",   "qie",   "qin",    "qing",  "qiong",  "qiu",   "qu",    "quan",  "que",   "qun",
        "ra",    "rai",   "ran",    "rang",  "rao",    "re",    "rei",    "ren",   "reng",  "rer",   "ri",    "rong",
        "rou",   "ru",    "rua",    "ruai",  "ruan",   "ruang", "rui",    "run",   "ruo",   "rv",    "ryi",   "sa",
        "sai",   "san",   "sang",   "sao",   "se",     "sei",   "sen",    "seng",  "ser",   "sha",   "shai",  "shan",
        "shang", "shao",  "she",    "shei",  "shen",   "sheng", "sher",   "shi",   "shong", "shou",  "shu",   "shua",
        "shuai", "shuan", "shuang", "shui",  "shun",   "shuo",  "shv",    "shyi",  "si",    "song",  "sou",   "su",
        "sua",   "suai",  "suan",   "suang", "sui",    "sun",   "suo",    "sv",    "syi",   "ta",    "tai",   "tan",
        "tang",  "tao",   "te",     "tei",   "ten",    "teng",  "ter",    "ti",    "tia",   "tian",  "tiang", "tiao",
        "tie",   "tin",   "ting",   "tiong", "tong",   "tou",   "tu",     "tua",   "tuai",  "tuan",  "tuang", "tui",
        "tun",   "tuo",   "tv",     "tve",   "wa",     "wai",   "wan",    "wang",  "wao",   "we",    "wei",   "wen",
        "weng",  "wer",   "wi",     "wo",    "wong",   "wou",   "wu",     "xi",    "xia",   "xian",  "xiang", "xiao",
        "xie",   "xin",   "xing",   "xiong", "xiu",    "xu",    "xuan",   "xue",   "xun",   "ya",    "yai",   "yan",
        "yang",  "yao",   "ye",     "yei",   "yi",     "yin",   "ying",   "yo",    "yong",  "you",   "yu",    "yuan",
        "yue",   "yun",   "ywu",    "za",    "zai",    "zan",   "zang",   "zao",   "ze",    "zei",   "zen",   "zeng",
        "zer",   "zha",   "zhai",   "zhan",  "zhang",  "zhao",  "zhe",    "zhei",  "zhen",  "zheng", "zher",  "zhi",
        "zhong", "zhou",  "zhu",    "zhua",  "zhuai",  "zhuan", "zhuang", "zhui",  "zhun",  "zhuo",  "zhv",   "zhyi",
        "zi",    "zong",  "zou",    "zu",    "zua",    "zuai",  "zuan",   "zuang", "zui",   "zun",   "zuo",   "zv",
        "zyi"};

    static bool isLetter(const QChar &c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '\''; }

    bool PinyinAnalyzer::contains(const QString &input) const { return pinyinSet.contains(input); }

    QList<LangNote> PinyinAnalyzer::split(const QString &input, const QString &g2pId) const {
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

    QString PinyinAnalyzer::randString() const { return getRandomElementFromSet(pinyinSet); }

} // namespace LangMgr
