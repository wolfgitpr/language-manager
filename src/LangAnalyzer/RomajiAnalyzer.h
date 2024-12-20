#ifndef ROMAJIANALYSIS_H
#define ROMAJIANALYSIS_H

#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{
    class RomajiAnalyzer final : public ILanguageFactory {
        Q_OBJECT
    public:
        explicit RomajiAnalyzer(const QString &id = "jpn-romaji", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setDisplayName(tr("Romaji"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // ROMAJIANALYSIS_H
