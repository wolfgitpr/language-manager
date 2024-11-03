#ifndef LANGCOMMON_H
#define LANGCOMMON_H

#include <QString>

#include <language-manager/LangGlobal.h>

struct LANG_MANAGER_EXPORT LangNote {
    QString lyric;
    QString syllable = QString();
    QString syllableRevised = QString();
    QStringList candidates = QStringList();
    QString language = "unknown";
    QString standard = "unknown";
    QString g2pId = "unknown";
    bool revised = false;
    bool error = false;

    LangNote() = default;

    explicit LangNote(QString lyric) : lyric(std::move(lyric)){};

    LangNote(QString lyric, QString language) : lyric(std::move(lyric)), language(std::move(language)){};
};
#endif // LANGCOMMON_H
