#ifndef UNKNOWN_H
#define UNKNOWN_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{

    class Unknown final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit Unknown(QObject *parent = nullptr);
        ~Unknown() override;

        QList<LangNote> convert(const QStringList &input) const override;

        QJsonObject config() override;
    };

} // namespace LangMgr

#endif // UNKNOWN_H
