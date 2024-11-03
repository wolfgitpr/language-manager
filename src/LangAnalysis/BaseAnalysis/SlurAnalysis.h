#ifndef SLURANALYSIS_H
#define SLURANALYSIS_H

#include "../BaseFactory/SingleCharFactory.h"

namespace LangMgr {

    class SlurAnalysis final : public SingleCharFactory {
        Q_OBJECT

    public:
        explicit SlurAnalysis(const QString &id = "slur", QObject *parent = nullptr)
            : SingleCharFactory(id, parent) {
            setDisplayName(tr("Slur"));
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // SLURANALYSIS_H