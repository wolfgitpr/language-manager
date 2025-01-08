#ifndef RESERVEDTOKENSG2P_H
#define RESERVEDTOKENSG2P_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr {

    class ReservedTokensG2p : public IG2pFactory {
        Q_OBJECT
    public:
        explicit ReservedTokensG2p(const QString &id = "reserved-token", QObject *parent = nullptr);
        ~ReservedTokensG2p() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;
    };

} // LangMgr

#endif //RESERVEDTOKENSG2P_H
