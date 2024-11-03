#ifndef KANA_H
#define KANA_H

#include <QObject>

#include <language-manager/IG2pFactory.h>

namespace LangMgr
{
    class JapaneseG2p final : public IG2pFactory {
        Q_OBJECT

    public:
        explicit JapaneseG2p(const QString &id = "jpn", const QString &categroy = "jpn", QObject *parent = nullptr);
        ~JapaneseG2p() override;

        [[nodiscard]] QList<LangNote> convert(const QStringList &input) const override;
    };

} // namespace LangMgr

#endif // KANA_H
