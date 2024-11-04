#include "CantoneseAnalyzer.h"

namespace LangMgr {
    bool CantoneseAnalyzer::contains(const QChar &c) const {
        return MandarinAnalyzer::contains(c);
    }

    QString CantoneseAnalyzer::randString() const {
        return MandarinAnalyzer::randString();
    }
} // LangMgr