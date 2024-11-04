#ifndef NUMBERANALYSIS_H
#define NUMBERANALYSIS_H

#include "../BaseFactory/MultiCharFactory.h"

namespace LangMgr {

    class NumberAnalyzer final : public MultiCharFactory {
        Q_OBJECT

    public:
        explicit NumberAnalyzer(QObject *parent = nullptr) : MultiCharFactory("number", parent) {
            setDisplayName(tr("Number"));
            setDiscardResult(true);
        }

        [[nodiscard]] bool contains(const QChar &c) const override;
        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // NUMBERANALYSIS_H