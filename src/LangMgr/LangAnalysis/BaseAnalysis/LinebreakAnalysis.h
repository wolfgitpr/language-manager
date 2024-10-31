#ifndef LINEBREAKANALYSIS_H
#define LINEBREAKANALYSIS_H

#include "../BaseFactory/SingleCharFactory.h"

namespace LangMgr
{
    class LinebreakAnalysis final : public SingleCharFactory {
        Q_OBJECT
    public:
        explicit LinebreakAnalysis(const QString &id = "linebreak", QObject *parent = nullptr) :
            SingleCharFactory(id, parent) {
            setAuthor(tr("Xiao Lang"));
            setDisplayName(tr("Linebreak"));
            setDescription(tr("Capture linebreaks."));
            setG2p("linebreak");
        }

        [[nodiscard]] bool contains(const QChar &c) const override;

        [[nodiscard]] QString randString() const override;
    };

} // namespace LangMgr

#endif // LINEBREAKANALYSIS_H
