#ifndef CHINESEANALYSIS_H
#define CHINESEANALYSIS_H

#include "BaseFactory/SingleCharFactory.h"

namespace LangMgr {

    class MandarinAnalyzer : public SingleCharFactory {
        Q_OBJECT
    public:
        explicit MandarinAnalyzer(const QString &id = "cmn", QObject *parent = nullptr)
            : SingleCharFactory(id, parent) {
            setDisplayName(tr("Mandarin"));
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // CHINESEANALYSIS_H
