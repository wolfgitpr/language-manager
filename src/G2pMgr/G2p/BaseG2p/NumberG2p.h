#ifndef NUMBER_H
#define NUMBER_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class NumberG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit NumberG2p(const QString &id = "number", const QString &categroy = "number", QObject *parent = nullptr);
        ~NumberG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new NumberG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // NUMBER_H
