#ifndef NUMBER_H
#define NUMBER_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class NumberG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit NumberG2p(const QString &id = "number", QObject *parent = nullptr);
        ~NumberG2p() override = default;
    };

} // namespace LangMgr

#endif // NUMBER_H
