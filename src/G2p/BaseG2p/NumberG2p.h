#ifndef NUMBER_H
#define NUMBER_H

#include "UnknownG2p.h"

namespace LangMgr
{
    class NumberG2p final : public UnknownG2p {
        Q_OBJECT
    public:
        explicit NumberG2p(const QString &id = "number", const QString &categroy = "number", QObject *parent = nullptr);
        ~NumberG2p() override = default;
    };

} // namespace LangMgr

#endif // NUMBER_H
