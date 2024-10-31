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
    };

} // namespace LangMgr

#endif // SPACE_H
