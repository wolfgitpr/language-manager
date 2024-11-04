#include "EnglishG2p.h"

#include "LangAnalyzer/EnglishAnalyzer.h"

namespace LangMgr
{
    EnglishG2p::EnglishG2p(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("English"));
        setDescription(tr("Greedy matching of consecutive English letters."));
        m_langFactory.insert("eng", new EnglishAnalyzer());
    }

    EnglishG2p::~EnglishG2p() = default;

    QList<LangNote> EnglishG2p::convert(const QStringList &input) const {
        QList<LangNote> result;
        for (auto &c : input) {
            const auto syllable = m_toLower ? c.toLower() : c;

            LangNote langNote;
            langNote.lyric = c;
            langNote.syllable = syllable;
            langNote.candidates = QStringList() << syllable;
            result.append(langNote);
        }
        return result;
    }

    QJsonObject EnglishG2p::defaultConfig() {
        QJsonObject config;
        config["toLower"] = m_toLower;
        config.insert("languageConfig", languageDefaultConfig());
        return config;
    }

    void EnglishG2p::loadG2pConfig(const QJsonObject &config) {
        if (config.contains("toLower"))
            m_toLower = config.value("toLower").toBool();
        m_config->insert("toLower", m_toLower);
    }
} // namespace LangMgr
