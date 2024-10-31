#include "English.h"

namespace LangMgr
{
    English::English(const QString &id, const QString &categroy, QObject *parent) : IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("English"));
        setDescription(tr("Greedy matching of consecutive English letters."));
    }

    English::~English() = default;

    QList<LangNote> English::convert(const QStringList &input) const {
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

    QJsonObject English::config() {
        QJsonObject config;
        config["toLower"] = m_toLower;
        return config;
    }

    void English::loadConfig(const QJsonObject &config) {
        if (config.contains("toLower"))
            m_toLower = config.value("toLower").toBool();
    }

    bool English::toLower() const { return m_toLower; }

    void English::setToLower(const bool &toLower) { m_toLower = toLower; }
} // namespace LangMgr
