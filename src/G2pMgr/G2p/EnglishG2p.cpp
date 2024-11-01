#include "EnglishG2p.h"

namespace LangMgr
{
    EnglishG2p::EnglishG2p(const QString &id, const QString &categroy, QObject *parent) : IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("English"));
        setDescription(tr("Greedy matching of consecutive English letters."));
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

    QJsonObject EnglishG2p::config() {
        QJsonObject config;
        config["toLower"] = m_toLower;
        return config;
    }

    void EnglishG2p::loadConfig(const QJsonObject &config) {
        if (config.contains("toLower"))
            m_toLower = config.value("toLower").toBool();
    }

    bool EnglishG2p::toLower() const { return m_toLower; }

    void EnglishG2p::setToLower(const bool &toLower) { m_toLower = toLower; }
} // namespace LangMgr
