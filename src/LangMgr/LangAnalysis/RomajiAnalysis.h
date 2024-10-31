#ifndef ROMAJIANALYSIS_H
#define ROMAJIANALYSIS_H

#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class RomajiAnalysis final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit RomajiAnalysis(const QString &id = "ja-romaji", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setAuthor(tr("Xiao Lang"));
            setDisplayName(tr("Romaji"));
            setDescription(tr("Capture Romaji words."));
            setCategory("ja");
            setG2p("ja-romaji");
        }

        bool initialize(QString &errMsg) override;

        void loadDict();

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId = "unknown") const override;

        [[nodiscard]] QString randString() const override;

    private:
        QSet<QString> romajiSet;
    };

} // namespace LangMgr

#endif // ROMAJIANALYSIS_H
