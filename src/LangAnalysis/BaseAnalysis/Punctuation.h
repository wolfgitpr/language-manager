#ifndef PUNCTUATION_H
#define PUNCTUATION_H

#include "../BaseFactory/SingleCharFactory.h"

namespace LangMgr {

    class Punctuation final : public SingleCharFactory {
        Q_OBJECT

    public:
        explicit Punctuation(const QString &id = "punctuation", QObject *parent = nullptr)
            : SingleCharFactory(id, parent) {
            setDisplayName(tr("Punctuation"));
            setDiscardResult(true);
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // PUNCTUATION_H