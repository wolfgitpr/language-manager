#include "Unknown.h"

namespace LangMgr
{
    Unknown::Unknown(const QString &id, const QString &categroy, QObject *parent) : IG2pFactory(id, categroy, parent) {
        setAuthor(tr("Xiao Lang"));
        setDisplayName(tr("Unknown"));
        setDescription(tr("Unknown language, no conversion required."));
    }

    Unknown::~Unknown() = default;

    QList<LangNote> Unknown::convert(const QStringList &input) const {
        QList<LangNote> result;
        for (const auto &i : input) {
            LangNote langNote;
            langNote.lyric = i;
            langNote.syllable = i;
            langNote.candidates = QStringList() << i;
            result.append(langNote);
        }
        return result;
    }

    QJsonObject Unknown::config() { return {}; }
} // namespace LangMgr
