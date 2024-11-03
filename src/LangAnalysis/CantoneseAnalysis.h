#ifndef CANTONESEANALYSIS_H
#define CANTONESEANALYSIS_H

#include "MandarinAnalysis.h"

namespace LangMgr {

    class CantoneseAnalysis final : public MandarinAnalysis {
        Q_OBJECT

    public:
        explicit CantoneseAnalysis(const QString &id = "yue", QObject *parent = nullptr)
            : MandarinAnalysis(id, parent) {
            setDisplayName(tr("Cantonese"));
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };
} // LangMgr

#endif // CANTONESEANALYSIS_H