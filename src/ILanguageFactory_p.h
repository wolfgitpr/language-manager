#ifndef ILANGUAGEFACTORY_P_H
#define ILANGUAGEFACTORY_P_H

#include <language-manager/ILanguageFactory.h>

#include <QJsonObject>
#include <QObject>

namespace LangMgr
{

    class ILanguageFactoryPrivate final : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ILanguageFactory)
    public:
        ILanguageFactoryPrivate();
        ~ILanguageFactoryPrivate() override;

        static void init();

        ILanguageFactory *q_ptr;

        QString id;
        QString displayName;

        bool enabled = true;
        bool discardResult = false;
    };

} // namespace LangMgr

#endif // ILANGUAGEFACTORY_P_H
