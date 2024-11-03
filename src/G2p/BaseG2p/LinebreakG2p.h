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
    };

} // namespace LangMgr

#endif // LINEBREAK_H
