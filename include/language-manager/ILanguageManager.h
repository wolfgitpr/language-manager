#ifndef ILANGUAGEMANAGER_H
#define ILANGUAGEMANAGER_H

#include <language-manager/IG2pFactory.h>
#include <language-manager/LangGlobal.h>

namespace LangMgr
{

    class ILanguageManagerPrivate;

    class LANG_MANAGER_EXPORT ILanguageManager final : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ILanguageManager)

    public:
        explicit ILanguageManager(QObject *parent = nullptr);
        ~ILanguageManager() override;

        ILanguageManager(const ILanguageManager &) = delete;
        ILanguageManager &operator=(const ILanguageManager &) = delete;

        static ILanguageManager *instance();

        bool initialize(const QJsonObject &args, QString &errMsg);
        bool initialized();

    public:
        [[nodiscard]] IG2pFactory *g2p(const QString &id) const;
        [[nodiscard]] QList<IG2pFactory *> g2ps() const;

        bool addG2p(IG2pFactory *factory);
        bool removeG2p(const IG2pFactory *factory);
        bool removeG2p(const QString &id);
        void clearG2ps();

        [[nodiscard]] QStringList defaultOrder() const;
        void setDefaultOrder(const QStringList &order);

        [[nodiscard]] QList<LangNote> split(const QString &input) const;

        void correct(const QList<LangNote *> &input, const QStringList &priorityG2pIds = {}) const;
        void convert(const QList<LangNote *> &input) const;

        [[nodiscard]] QString analysis(const QString &input, const QStringList &priorityG2pIds = {}) const;
        [[nodiscard]] QStringList analysis(const QStringList &input, const QStringList &priorityG2pIds = {}) const;

    private:
        explicit ILanguageManager(ILanguageManagerPrivate &d, QObject *parent = nullptr);

        QScopedPointer<ILanguageManagerPrivate> d_ptr;
    };

} // namespace LangMgr

#endif // ILANGUAGEMANAGER_H
