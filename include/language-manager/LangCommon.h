#ifndef LANGCOMMON_H
#define LANGCOMMON_H

#include <QString>

#include <language-manager/LangGlobal.h>

struct LANG_MANAGER_EXPORT LangNote {
    QString lyric;
    QString syllable = QString();
    QString syllableRevised = QString();
    QStringList candidates = QStringList();
    QString standard = "unknown";
    QString g2pId = "unknown";
    bool revised = false;
    bool error = false;

    LangNote() = default;

    explicit LangNote(QString lyric) : lyric(std::move(lyric)) {}
};
#endif // LANGCOMMON_H
