#ifndef CANTONESE_H
#define CANTONESE_H

#include <QObject>

#include <cpp-pinyin/Jyutping.h>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class Cantonese final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit Cantonese(const QString &id = "yue", const QString &categroy = "yue", QObject *parent = nullptr);
        ~Cantonese() override;

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
