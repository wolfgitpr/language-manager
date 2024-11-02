#ifndef SLUR_H
#define SLUR_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class SlurG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit SlurG2p(const QString &id = "slur", const QString &categroy = "slur", QObject *parent = nullptr);
        ~SlurG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new SlurG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // SLUR_H
