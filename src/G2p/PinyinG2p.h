#ifndef PINYIN_H
#define PINYIN_H

#include "EnglishG2p.h"

namespace LangMgr
{
    class PinyinG2p final : public EnglishG2p {
        Q_OBJECT
    public:
        explicit PinyinG2p(const QString &id = "cmn-pinyin", const QString &categroy = "cmn",
                           QObject *parent = nullptr);
        ~PinyinG2p() override = default;
    };

} // namespace LangMgr

#endif // PINYIN_H
