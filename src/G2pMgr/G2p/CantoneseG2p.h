#ifndef CANTONESE_H
#define CANTONESE_H

#include <QObject>

#include <cpp-pinyin/Jyutping.h>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class CantoneseG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit CantoneseG2p(const QString &id = "yue", const QString &categroy = "yue", QObject *parent = nullptr);
        ~CantoneseG2p() override;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new CantoneseG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }

        bool initialize(QString &errMsg) override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject config() override;
        void loadConfig(const QJsonObject &config) override;

        [[nodiscard]] bool tone() const;
        void setTone(const bool &tone);

    private:
        std::unique_ptr<Pinyin::Jyutping> m_cantonese;

        bool m_tone = false;
    };

} // namespace LangMgr

#endif // CANTONESE_H
