#ifndef JYUTPING_H
#define JYUTPING_H

#include "EnglishG2p.h"

namespace LangMgr
{
    class Jyutping final : public EnglishG2p {
        Q_OBJECT
    public:
        explicit Jyutping(const QString &id = "yue-jyutping", const QString &categroy = "yue",
                          QObject *parent = nullptr);
        ~Jyutping() override = default;
    };

} // namespace LangMgr

#endif // JYUTPING_H
