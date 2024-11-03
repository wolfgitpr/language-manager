#ifndef KANAANALYSIS_H
#define KANAANALYSIS_H

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class KanaAnalysis final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit KanaAnalysis(QObject *parent = nullptr) : ILanguageFactory("jpn-kana", parent) {
            setDisplayName(tr("Kana"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;
        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId = "unknown") const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // KANAANALYSIS_H
