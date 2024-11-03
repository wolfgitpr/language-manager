#ifndef PUNCTUATION_H
#define PUNCTUATION_H

#include "../BaseFactory/SingleCharFactory.h"

namespace LangMgr {

    class Punctuation final : public SingleCharFactory {
        Q_OBJECT

    public:
        explicit Punctuation(const QString &id = "punctuation", QObject *parent = nullptr)
            : SingleCharFactory(id, parent) {
            setAuthor(tr("Xiao Lang"));
            setDisplayName(tr("Punctuation"));
            setDescription(tr("Capture punctuations."));
            setDiscardResult(true);
            setG2p("punctuation");
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // LangMgr

#endif // PUNCTUATION_H