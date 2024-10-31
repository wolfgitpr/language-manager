#ifndef ENGLISH_H
#define ENGLISH_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class English final : public IG2pFactory {
        Q_OBJECT

    public:
        explicit English(QObject *parent = nullptr);
        ~English() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject config() override;
        void loadConfig(const QJsonObject &config) override;

        [[nodiscard]] bool toLower() const;
        void setToLower(const bool &toLower);

    private:
        bool m_toLower = false;
    };

} // namespace LangMgr

#endif // ENGLISH_H
