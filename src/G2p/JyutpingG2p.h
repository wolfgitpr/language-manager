#ifndef JYUTPING_H
#define JYUTPING_H

#include "EnglishG2p.h"

namespace LangMgr
{
    class JyutpingG2p final : public EnglishG2p {
        Q_OBJECT
    public:
        explicit JyutpingG2p(const QString &id = "yue-jyutping", const QString &categroy = "yue",
                             QObject *parent = nullptr);
        ~JyutpingG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new JyutpingG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // JYUTPING_H
