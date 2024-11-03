#ifndef LINEBREAK_H
#define LINEBREAK_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class LinebreakG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit LinebreakG2p(const QString &id = "linebreak", const QString &categroy = "linebreak",
                              QObject *parent = nullptr);
        ~LinebreakG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new LinebreakG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // LINEBREAK_H
