#ifndef PUNCTUATIONG2P_H
#define PUNCTUATIONG2P_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class PunctuationG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit PunctuationG2p(const QString &id = "punctuation", QObject *parent = nullptr);
        ~PunctuationG2p() override = default;
    };

} // namespace LangMgr

#endif // PUNCTUATIONG2P_H
