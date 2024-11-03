#ifndef ROMAJI_H
#define ROMAJI_H

#include "EnglishG2p.h"

namespace LangMgr
{
    class RomajiG2p final : public EnglishG2p {
        Q_OBJECT
    public:
        explicit RomajiG2p(const QString &id = "ja-romaji", const QString &categroy = "ja", QObject *parent = nullptr);
        ~RomajiG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new RomajiG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // ROMAJI_H
