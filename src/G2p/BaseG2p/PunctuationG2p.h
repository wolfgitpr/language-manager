#ifndef PUNCTUATIONG2P_H
#define PUNCTUATIONG2P_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class PunctuationG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit PunctuationG2p(const QString &id = "punctuation", const QString &categroy = "punctuation",
                                QObject *parent = nullptr);
        ~PunctuationG2p() override = default;

        [[nodiscard]] IG2pFactory *clone(const QString &id, const QString &categroy, QObject *parent) const override {
            const auto factory = new PunctuationG2p(id, categroy, parent);
            factory->setBase(false);
            return factory;
        }
    };

} // namespace LangMgr

#endif // PUNCTUATIONG2P_H
