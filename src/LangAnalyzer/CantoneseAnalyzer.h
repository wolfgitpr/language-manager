#ifndef CANTONESEANALYSIS_H
#define CANTONESEANALYSIS_H

#include "MandarinAnalyzer.h"

namespace LangMgr {

    class CantoneseAnalyzer final : public MandarinAnalyzer {
        Q_OBJECT

    public:
        explicit CantoneseAnalyzer(const QString &id = "yue", QObject *parent = nullptr)
            : MandarinAnalyzer(id, parent) {
            setDisplayName(tr("Cantonese"));
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };
} // LangMgr

#endif // CANTONESEANALYSIS_H