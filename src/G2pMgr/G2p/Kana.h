#ifndef KANA_H
#define KANA_H

#include <QObject>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class KanaG2p final : public IG2pFactory {
        Q_OBJECT

    public:
        explicit KanaG2p(QObject *parent = nullptr);
        ~KanaG2p() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject config() override;
    };

} // namespace LangMgr

#endif // KANA_H
