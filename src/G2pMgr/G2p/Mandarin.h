#ifndef MANDARIN_H
#define MANDARIN_H

#include <QObject>

#include <cpp-pinyin/Pinyin.h>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class Mandarin final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit Mandarin(QObject *parent = nullptr);
        ~Mandarin() override;

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
