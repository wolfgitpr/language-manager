#ifndef LINEBREAK_H
#define LINEBREAK_H

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class LinebreakG2p final : public IG2pFactory {
        Q_OBJECT
    public:
        explicit LinebreakG2p(const QString &id = "linebreak", QObject *parent = nullptr);
        ~LinebreakG2p() override = default;
    };

} // namespace LangMgr

#endif // LINEBREAK_H
