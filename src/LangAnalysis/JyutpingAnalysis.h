#ifndef JYUTPINGANALYSIS_H
#define JYUTPINGANALYSIS_H


#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class JyutpingAnalysis final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit JyutpingAnalysis(const QString &id = "yue-jyutping", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setDisplayName(tr("Jyutping"));
        }

        bool initialize(QString &errMsg) override;

        void loadDict();

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;

    private:
        QSet<QString> jyutpingSet;
    };

} // namespace LangMgr

#endif // JYUTPINGANALYSIS_H