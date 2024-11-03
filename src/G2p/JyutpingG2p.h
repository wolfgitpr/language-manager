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
    };

} // namespace LangMgr

#endif // JYUTPING_H
