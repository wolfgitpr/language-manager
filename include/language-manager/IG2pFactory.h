#ifndef IG2PFACTORY_H
#define IG2PFACTORY_H

#include <QJsonObject>
#include <QObject>

#include <language-manager/LangCommon.h>
#include <language-manager/LangGlobal.h>

#include <language-manager/ILanguageFactory.h>

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

        [[nodiscard]] virtual QList<LangNote> split(const QString &input) const;
        [[nodiscard]] QList<LangNote> split(const QList<LangNote> &input) const;
        [[nodiscard]] QString analysis(const QString &input) const;
        void correct(const QList<LangNote *> &input) const;

        [[nodiscard]] virtual QList<LangNote> convert(const QStringList &input) const;

        [[nodiscard]] virtual QString randString() const;

        virtual QJsonObject defaultConfig();
        virtual QJsonObject languageDefaultConfig();

        virtual QJsonObject config();

        virtual void loadG2pConfig(const QJsonObject &config);

        [[nodiscard]] QJsonObject languageConfig() const;
        void loadLanguageConfig(const QJsonObject &config);

    public:
        [[nodiscard]] QString id() const;

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

        QJsonObject *m_config = new QJsonObject();
        QMap<QString, ILanguageFactory *> m_langFactory;

        friend class ILanguageManager;
    };

} // namespace LangMgr

#endif // IG2PFACTORY_H
