#include "SlurAnalyzer.h"

namespace LangMgr {
    bool SlurAnalyzer::contains(const QChar &c) const {
        return c == '-';
    }

    QString SlurAnalyzer::randString() const {
        return "-";
    }

} // LangMgr