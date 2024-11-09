#ifndef PINYINANALYSIS_H
#define PINYINANALYSIS_H

#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class PinyinAnalyzer final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit PinyinAnalyzer(const QString &id = "cmn-pinyin", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setDisplayName(tr("Pinyin"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // PINYINANALYSIS_H
