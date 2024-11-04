#ifndef UNKNOWANALYSIS_H
#define UNKNOWANALYSIS_H

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class UnknownAnalyzer final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit UnknownAnalyzer(QObject *parent = nullptr) : ILanguageFactory("unknown", parent) {
            setDisplayName(tr("Unknown"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;
        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // UNKNOWANALYSIS_H
