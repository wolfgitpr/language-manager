#ifndef ILANGUAGEMANAGER_P_H
#define ILANGUAGEMANAGER_P_H

#include <QMap>
#include <QObject>

#include <language-manager/IG2pFactory.h>
#include <language-manager/ILanguageManager.h>

namespace LangMgr
{
    class ILanguageManagerPrivate final : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ILanguageManager)

    public:
        ILanguageManagerPrivate();
        ~ILanguageManagerPrivate() override;

        [[nodiscard]] QList<IG2pFactory *> priorityG2ps(const QStringList &priorityG2pIds = {}) const;

        bool initialized = false;

        ILanguageManager *q_ptr;

        QStringList defaultOrder = {"cmn",  "yue",         "jpn",    "eng",       "space",
                                    "slur", "punctuation", "number", "linebreak", "unknown"};

        QMap<QString, IG2pFactory *> g2ps;

        QString m_pinyinDictPath;
    };

} // namespace LangMgr

#endif // ILANGUAGEMANAGER_P_H
