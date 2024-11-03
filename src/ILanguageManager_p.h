#ifndef ILANGUAGEMANAGER_P_H
#define ILANGUAGEMANAGER_P_H

#include <QMap>
#include <QObject>

#include <language-manager/IG2pFactory.h>
#include <language-manager/ILanguageManager.h>

namespace LangMgr
{
    struct Analysiser {
        ILanguageFactory *analysis;
        QString g2pId = "unknown";
    };

    class ILanguageManagerPrivate final : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ILanguageManager)

    public:
        ILanguageManagerPrivate();
        ~ILanguageManagerPrivate() override;

        [[nodiscard]] QList<Analysiser> priorityLanguages(const QStringList &priorityG2pIds = {}) const;

        bool initialized = false;

        ILanguageManager *q_ptr;

        QStringList defaultOrder = {"cmn",       "cmn-pinyin", "yue",    "yue-jyutping", "ja-kana",
                                    "ja-romaji", "en",         "space",  "slur",         "punctuation",
                                    "number",    "linebreak",  "unknown"};

        QMap<QString, ILanguageFactory *> languages;
    };

} // namespace LangMgr

#endif // ILANGUAGEMANAGER_P_H
