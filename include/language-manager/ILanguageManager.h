#ifndef ILANGUAGEMANAGER_H
#define ILANGUAGEMANAGER_H

#include <language-manager/ILanguageFactory.h>
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

        bool initialize(QString &errMsg);
        bool initialized();

    public:
        [[nodiscard]] ILanguageFactory *language(const QString &id) const;
        [[nodiscard]] QList<ILanguageFactory *> languages() const;

        bool addLanguage(ILanguageFactory *factory);
        bool removeLanguage(const ILanguageFactory *factory);
        bool removeLanguage(const QString &id);
        void clearLanguages();

        [[nodiscard]] QStringList defaultOrder() const;
        void setDefaultOrder(const QStringList &order);

        [[nodiscard]] QList<LangNote> split(const QString &input) const;

        void correct(const QList<LangNote *> &input, const QStringList &priorityG2pIds = {}) const;
        static void convert(const QList<LangNote *> &input);

        [[nodiscard]] QString analysis(const QString &input, const QStringList &priorityG2pIds = {}) const;
        [[nodiscard]] QStringList analysis(const QStringList &input, const QStringList &priorityG2pIds = {}) const;

    private:
        explicit ILanguageManager(ILanguageManagerPrivate &d, QObject *parent = nullptr);

        QScopedPointer<ILanguageManagerPrivate> d_ptr;
    };

} // namespace LangMgr

#endif // ILANGUAGEMANAGER_H
