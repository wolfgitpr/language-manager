#ifndef ROMAJI_H
#define ROMAJI_H

#include "EnglishG2p.h"

namespace LangMgr
{
    class Romaji final : public EnglishG2p {
        Q_OBJECT
    public:
        explicit Romaji(const QString &id = "ja-romaji", const QString &categroy = "ja", QObject *parent = nullptr);
        ~Romaji() override = default;
    };

} // namespace LangMgr

#endif // ROMAJI_H
