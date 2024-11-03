#ifndef IG2PFACTORYPRIVATE_H
#define IG2PFACTORYPRIVATE_H

#include <QObject>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class IG2pFactoryPrivate final : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(IG2pFactory)
    public:
        IG2pFactoryPrivate();
        ~IG2pFactoryPrivate() override;

        static void init();

        IG2pFactory *q_ptr;

        QString id;
        bool base = true;
        QString displayName;
        QString categroy;
        QString author;
        QString description;

        QString configId = "0";
    };

} // namespace LangMgr

#endif // IG2PFACTORYPRIVATE_H
