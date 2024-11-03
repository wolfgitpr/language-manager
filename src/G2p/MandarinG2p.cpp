#include "MandarinG2p.h"

#include "LangAnalysis/MandarinAnalysis.h"
#include "LangAnalysis/PinyinAnalysis.h"

namespace LangMgr
{
    MandarinG2p::MandarinG2p(const QString &id, const QString &categroy, QObject *parent) :
        IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Mandarin"));
        setDescription(tr("Using Pinyin as the phonetic notation method."));
        m_langFactory.insert("cmn", new MandarinAnalysis());
        m_langFactory.insert("cmn-pinyin", new PinyinAnalysis());
    }

    MandarinG2p::~MandarinG2p() = default;

    bool MandarinG2p::initialize(QString &errMsg) {
        m_mandarin = std::make_unique<Pinyin::Pinyin>();
        if (!m_mandarin->initialized()) {
            errMsg = tr("Failed to initialize Mandarin G2P");
            return false;
        }
        m_config->insert("0", defaultConfig());
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

    QList<LangNote> MandarinG2p::convert(const QStringList &input) const {
        const auto style = m_tone ? Pinyin::ManTone::Style::TONE3 : Pinyin::ManTone::Style::NORMAL;

        QList<LangNote> result;
        const auto g2pRes = m_mandarin->hanziToPinyin(toStdVector(input), style);
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

    QJsonObject MandarinG2p::defaultConfig() {
        QJsonObject config;
        config["tone"] = m_tone;
        config.insert("languageConfig", languageDefaultConfig());
        return config;
    }

    void MandarinG2p::loadG2pConfig(const QJsonObject &config) {
        if (config.contains("tone")) {
            m_tone = config.value("tone").toBool();
        }
    }

    bool MandarinG2p::tone() const { return m_tone; }

    void MandarinG2p::setTone(const bool &tone) { m_tone = tone; }
} // namespace LangMgr
