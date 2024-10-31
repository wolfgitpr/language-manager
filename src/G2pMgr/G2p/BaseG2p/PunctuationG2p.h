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
    };

} // namespace LangMgr

#endif // PUNCTUATIONG2P_H
