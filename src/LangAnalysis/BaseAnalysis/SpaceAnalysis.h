#ifndef SPACEANALYSIS_H
#define SPACEANALYSIS_H

#include "../BaseFactory/SingleCharFactory.h"

namespace LangMgr {

    class SpaceAnalysis final : public SingleCharFactory {
        Q_OBJECT

    public:
        explicit SpaceAnalysis(const QString &id = "space", QObject *parent = nullptr)
            : SingleCharFactory(id, parent) {
            setDisplayName(tr("Space"));
            setDiscardResult(true);
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // SPACEANALYSIS_H