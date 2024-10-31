#ifndef PINYINANALYSIS_H
#define PINYINANALYSIS_H

#include <QSet>

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class PinyinAnalysis final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit PinyinAnalysis(const QString &id = "cmn-pinyin", QObject *parent = nullptr) :
            ILanguageFactory(id, parent) {
            setAuthor(tr("Xiao Lang"));
            setDisplayName(tr("Pinyin"));
            setDescription(tr("Capture Pinyin words."));
            setCategory("cmn");
            setG2p("unknown");
        }

        bool initialize(QString &errMsg) override;

        void loadDict();

        [[nodiscard]] bool contains(const QString &input) const override;

        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId = "unknown") const override;

        [[nodiscard]] QString randString() const override;

    private:
        QSet<QString> pinyinSet;
    };

} // namespace LangMgr

#endif // PINYINANALYSIS_H
