#ifndef ILANGUAGEMANAGER_H
#define ILANGUAGEMANAGER_H

#include <QObject>

#include <language-manager/LangGlobal.h>
#include <language-manager/Singleton.h>
#include <language-manager/ILanguageFactory.h>

namespace LangMgr {

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

        void correct(const QList<LangNote *> &input, const QStringList &priorityList = {}) const;
        void convert(const QList<LangNote *> &input) const;

        [[nodiscard]] QString analysis(const QString &input,
                                       const QStringList &priorityList = {}) const;
        [[nodiscard]] QStringList analysis(const QStringList &input,
                                           const QStringList &priorityList = {}) const;

    private:
        [[nodiscard]] QList<ILanguageFactory *>
            priorityLanguages(const QStringList &priorityList = {}) const;

        explicit ILanguageManager(ILanguageManagerPrivate &d, QObject *parent = nullptr);

        QScopedPointer<ILanguageManagerPrivate> d_ptr;
    };

} // LangMgr

#endif // ILANGUAGEMANAGER_H