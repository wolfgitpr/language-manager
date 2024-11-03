#ifndef UNKNOWN_H
#define UNKNOWN_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class UnknownG2p : public IG2pFactory {
        Q_OBJECT
    public:
        explicit UnknownG2p(const QString &id = "unknown", const QString &categroy = "unknown",
                            QObject *parent = nullptr);
        ~UnknownG2p() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject config() override;
    };

} // namespace LangMgr

#endif // UNKNOWN_H
