#ifndef IG2PFACTORY_H
#define IG2PFACTORY_H

#include <QJsonObject>
#include <QObject>

#include <language-manager/LangCommon.h>
#include <language-manager/LangGlobal.h>

namespace LangMgr
{

    class IG2pFactoryPrivate;

    class LANG_MANAGER_EXPORT IG2pFactory : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(IG2pFactory)
    public:
        explicit IG2pFactory(const QString &id, const QString &categroy = "", QObject *parent = nullptr);
        ~IG2pFactory() override;

        virtual bool initialize(QString &errMsg);

        virtual IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const = 0;

        [[nodiscard]] LangNote convert(const QString &input) const;
        [[nodiscard]] virtual QList<LangNote> convert(const QStringList &input) const;

        virtual QJsonObject config();
        virtual void loadConfig(const QJsonObject &config);

    public:
        [[nodiscard]] QString id() const;

        [[nodiscard]] bool base() const;
        void setBase(const bool &base);

        [[nodiscard]] QString displayName() const;
        void setDisplayName(const QString &displayName);

        [[nodiscard]] QString category() const;
        void setCategory(const QString &category);

        [[nodiscard]] QString author() const;
        void setAuthor(const QString &author);

        [[nodiscard]] QString description() const;
        void setDescription(const QString &description);

    protected:
        IG2pFactory(IG2pFactoryPrivate &d, const QString &id, const QString &categroy = "", QObject *parent = nullptr);

        QScopedPointer<IG2pFactoryPrivate> d_ptr;

        friend class IG2pManager;
    };

} // namespace LangMgr

#endif // IG2PFACTORY_H
