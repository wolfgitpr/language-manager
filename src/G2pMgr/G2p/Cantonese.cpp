#include "Cantonese.h"

namespace LangMgr
{
    Cantonese::Cantonese(QObject *parent) : IG2pFactory("yue", parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Cantonese"));
        setDescription(tr("Using Cantonese Pinyin as the phonetic notation method."));
    }

    Cantonese::~Cantonese() = default;

    bool Cantonese::initialize(QString &errMsg) {
        m_cantonese = std::make_unique<Pinyin::Jyutping>();
        if (!m_cantonese->initialized()) {
            errMsg = tr("Failed to initialize Cantonese G2P");
            return false;
        }
        return true;
    }

    static std::vector<std::string> toStdVector(const QStringList &input) {
        std::vector<std::string> result;
        result.reserve(input.size());
        for (const auto &str : input)
            result.push_back(str.toUtf8().toStdString());
        return result;
    }

    static QStringList fromStdVector(const std::vector<std::string> &input) {
        QStringList result;
        for (const auto &str : input)
            result.append(QString::fromUtf8(str.c_str()));
        return result;
    }

    QList<LangNote> Cantonese::convert(const QStringList &input) const {
        const auto style = m_tone ? Pinyin::CanTone::Style::TONE3 : Pinyin::CanTone::Style::NORMAL;

        QList<LangNote> result;
        const auto g2pRes = m_cantonese->hanziToPinyin(toStdVector(input), style);
        for (int i = 0; i < g2pRes.size(); i++) {
            LangNote langNote;
            langNote.lyric = input[i];
            langNote.syllable = QString::fromUtf8(g2pRes[i].pinyin.c_str());
            langNote.candidates = fromStdVector(g2pRes[i].candidates);
            langNote.error = g2pRes[i].error;
            result.append(langNote);
        }
        return result;
    }

    QJsonObject Cantonese::config() {
        QJsonObject config;
        config["tone"] = m_tone;
        return config;
    }

    void Cantonese::loadConfig(const QJsonObject &config) {
        if (config.contains("tone"))
            m_tone = config.value("tone").toBool();
    }

    bool Cantonese::tone() const { return m_tone; }

    void Cantonese::setTone(const bool &tone) { m_tone = tone; }
} // namespace LangMgr
