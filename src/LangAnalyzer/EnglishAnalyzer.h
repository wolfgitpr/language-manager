#ifndef ENGLISHANALYSIS_H
#define ENGLISHANALYSIS_H

#include "BaseFactory/MultiCharFactory.h"

namespace LangMgr
{
    class EnglishAnalyzer final : public MultiCharFactory {
        Q_OBJECT

    public:
        explicit EnglishAnalyzer(QObject *parent = nullptr) : MultiCharFactory("eng", parent) {
            setDisplayName(tr("English"));
        }

        [[nodiscard]] bool contains(const QChar &c) const override;
        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // ENGLISHANALYSIS_H
