#ifndef KANAANALYSIS_H
#define KANAANALYSIS_H

#include <language-manager/ILanguageFactory.h>

namespace LangMgr
{

    class KanaAnalyzer final : public ILanguageFactory {
        Q_OBJECT

    public:
        explicit KanaAnalyzer(QObject *parent = nullptr) : ILanguageFactory("jpn-kana", parent) {
            setDisplayName(tr("Kana"));
        }

        [[nodiscard]] bool contains(const QString &input) const override;
        [[nodiscard]] QList<LangNote> split(const QString &input, const QString &g2pId) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // KANAANALYSIS_H
