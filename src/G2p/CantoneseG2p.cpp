#include "CantoneseG2p.h"

#include "LangAnalysis/CantoneseAnalysis.h"
#include "LangAnalysis/JyutpingAnalysis.h"

namespace LangMgr
{
    CantoneseG2p::CantoneseG2p(const QString &id, const QString &categroy, QObject *parent) :
        IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Cantonese"));
        setDescription(tr("Using Cantonese Pinyin as the phonetic notation method."));
        m_langFactory.insert("yue", new CantoneseAnalysis());
        m_langFactory.insert("yue-jyutping", new JyutpingAnalysis());
    }

    CantoneseG2p::~CantoneseG2p() = default;

    bool CantoneseG2p::initialize(QString &errMsg) {
        m_cantonese = std::make_unique<Pinyin::Jyutping>();
        if (!m_cantonese->initialized()) {
            errMsg = tr("Failed to initialize Cantonese G2P");
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

    QList<LangNote> CantoneseG2p::convert(const QStringList &input) const {
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

    QJsonObject CantoneseG2p::defaultConfig() {
        QJsonObject config;
        config["tone"] = m_tone;
        config.insert("languageConfig", languageDefaultConfig());
        return config;
    }

    void CantoneseG2p::loadG2pConfig(const QJsonObject &config, const QString &configId) {
        if (config.contains("tone"))
            m_tone = config.value("tone").toBool();

        if (m_config && m_config->contains(configId)) {
            QJsonObject langConfigItem = m_config->value(configId).toObject();

            langConfigItem["tone"] = m_tone;
            (*m_config)[configId] = langConfigItem;
        }
    }

    bool CantoneseG2p::tone() const { return m_tone; }

    void CantoneseG2p::setTone(const bool &tone) { m_tone = tone; }
} // namespace LangMgr
