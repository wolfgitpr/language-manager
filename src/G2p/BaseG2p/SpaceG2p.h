#ifndef SPACE_H
#define SPACE_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class SpaceG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit SpaceG2p(const QString &id = "space", const QString &categroy = "space", QObject *parent = nullptr);
        ~SpaceG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new SpaceG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // SPACE_H
