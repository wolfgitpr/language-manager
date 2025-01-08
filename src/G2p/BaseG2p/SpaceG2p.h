#ifndef SPACE_H
#define SPACE_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class SpaceG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit SpaceG2p(const QString &id = "space", QObject *parent = nullptr);
        ~SpaceG2p() override = default;
    };

} // namespace LangMgr

#endif // SPACE_H
