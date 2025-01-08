#ifndef SLUR_H
#define SLUR_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class SlurG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit SlurG2p(const QString &id = "slur", QObject *parent = nullptr);
        ~SlurG2p() override = default;
    };

} // namespace LangMgr

#endif // SLUR_H
