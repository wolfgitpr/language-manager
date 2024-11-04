#include "LinebreakAnalyzer.h"

namespace LangMgr {
    bool LinebreakAnalyzer::contains(const QChar &c) const {
        return c == QChar::LineFeed || c == QChar::LineSeparator || c == QChar::ParagraphSeparator;
    }

    QString LinebreakAnalyzer::randString() const {
        return {QChar::LineFeed};
    }

} // LangMgr