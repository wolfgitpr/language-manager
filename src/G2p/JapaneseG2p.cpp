#include "JapaneseG2p.h"

#include <cpp-kana/Kana.h>

#include "LangAnalysis/KanaAnalysis.h"
#include "LangAnalysis/RomajiAnalysis.h"

namespace LangMgr
{
    JapaneseG2p::JapaneseG2p(const QString &id, const QString &categroy, QObject *parent) :
        IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Japanese"));
        setDescription(tr("Kana to Romaji converter."));
        m_langFactory.insert("jpn-kana", new KanaAnalysis());
        m_langFactory.insert("jpn-romaji", new RomajiAnalysis());
    }

    JapaneseG2p::~JapaneseG2p() = default;

    static std::vector<std::string> toStdVector(const QStringList &input) {
        std::vector<std::string> result;
        result.reserve(input.size());
        for (const auto &str : input)
            result.push_back(str.toUtf8().toStdString());
        return result;
    }

    QList<LangNote> JapaneseG2p::convert(const QStringList &input) const {
        QList<LangNote> result;
        const auto g2pRes = Kana::kanaToRomaji(toStdVector(input), Kana::Error::Default, false);
        for (int i = 0; i < g2pRes.size(); i++) {
            LangNote langNote;
            langNote.lyric = input[i];
            langNote.syllable = QString::fromUtf8(g2pRes[i].romaji.c_str());
            langNote.candidates = QStringList() << langNote.syllable;
            langNote.error = g2pRes[i].error;
            result.append(langNote);
        }
        return result;
    }
} // namespace LangMgr