#ifndef ILANGUAGEMANAGER_P_H
#define ILANGUAGEMANAGER_P_H

#include <QMap>
#include <QObject>

#include <language-manager/IG2pFactory.h>
#include <language-manager/ILanguageManager.h>

namespace LangMgr
{
    struct G2p {
        IG2pFactory *g2p;
        QString config = "0";
    };

    class ILanguageManagerPrivate final : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ILanguageManager)

    public:
        ILanguageManagerPrivate();
        ~ILanguageManagerPrivate() override;

        [[nodiscard]] QList<G2p> priorityG2ps(const QStringList &priorityG2pIds = {}) const;

        bool initialized = false;

        ILanguageManager *q_ptr;

        QStringList defaultOrder = {"cmn",  "yue",         "yue-jyutping", "jpn",       "eng",    "space",
                                    "slur", "punctuation", "number",       "linebreak", "unknown"};

        QMap<QString, IG2pFactory *> g2ps;

        QString m_pinyinDictPath;

        QStringList baseG2p = {"slur", "linebreak", "number", "space", "punctuation"};
    };

} // namespace LangMgr

#endif // ILANGUAGEMANAGER_P_H
