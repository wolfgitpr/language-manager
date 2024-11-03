#ifndef ILANGUAGEFACTORY_H
#define ILANGUAGEFACTORY_H

#include <QObject>

#include <language-manager/LangCommon.h>
#include <language-manager/LangGlobal.h>

namespace LangMgr
{

    class ILanguageFactoryPrivate;

    class LANG_MANAGER_EXPORT ILanguageFactory : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ILanguageFactory)
    public:
        explicit ILanguageFactory(const QString &id, QObject *parent = nullptr);
        ~ILanguageFactory() override;

        virtual bool initialize(QString &errMsg);

        [[nodiscard]] virtual bool contains(const QChar &c) const;
        [[nodiscard]] virtual bool contains(const QString &input) const;

        [[nodiscard]] virtual QString randString() const;

        [[nodiscard]] virtual QList<LangNote> split(const QString &input, const QString &g2pId) const;
        [[nodiscard]] QList<LangNote> split(const QList<LangNote> &input, const QString &g2pId) const;
        [[nodiscard]] QString analysis(const QString &input) const;
        void correct(const QList<LangNote *> &input, const QString &g2pId) const;

        virtual void loadConfig(const QJsonObject &config);
        [[nodiscard]] virtual QJsonObject exportConfig() const;

    public:
        [[nodiscard]] QString id() const;

        [[nodiscard]] QString displayName() const;
        void setDisplayName(const QString &name);

        [[nodiscard]] bool enabled() const;
        void setEnabled(const bool &enable);

        [[nodiscard]] bool discardResult() const;
        void setDiscardResult(const bool &discard);

    protected:
        ILanguageFactory(ILanguageFactoryPrivate &d, const QString &id, QObject *parent = nullptr);

        QScopedPointer<ILanguageFactoryPrivate> d_ptr;

        friend class ILanguageManager;

    }; // ILanguageFactory

} // namespace LangMgr

#endif // ILANGUAGEFACTORY_H
