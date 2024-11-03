#ifndef MANDARIN_H
#define MANDARIN_H

#include <QObject>

#include <cpp-pinyin/Pinyin.h>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class MandarinG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit MandarinG2p(const QString &id = "cmn", const QString &categroy = "cmn", QObject *parent = nullptr);
        ~MandarinG2p() override;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new MandarinG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }

        bool initialize(QString &errMsg) override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;
        void loadConfig(const QJsonObject &config) override;

        QJsonObject config() override;

        [[nodiscard]] bool tone() const;
        void setTone(const bool &tone);

    private:
        std::unique_ptr<Pinyin::Pinyin> m_mandarin;

        bool m_tone = false;
    };
} // namespace LangMgr

#endif // MANDARIN_H
