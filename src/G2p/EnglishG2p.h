#ifndef ENGLISH_H
#define ENGLISH_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class EnglishG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit EnglishG2p(const QString &id = "eng", const QString &categroy = "eng", QObject *parent = nullptr);
        ~EnglishG2p() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject defaultConfig() override;
        void loadConfig(const QJsonObject &config) override;

        [[nodiscard]] bool toLower() const;
        void setToLower(const bool &toLower);

    private:
        bool m_toLower = false;
    };

} // namespace LangMgr

#endif // ENGLISH_H
