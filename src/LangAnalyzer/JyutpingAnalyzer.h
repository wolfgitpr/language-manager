#ifndef JYUTPINGANALYSIS_H
#define JYUTPINGANALYSIS_H


#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class JyutpingAnalyzer final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit JyutpingAnalyzer(const QString &id = "yue-jyutping", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setDisplayName(tr("Jyutping"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // JYUTPINGANALYSIS_H
