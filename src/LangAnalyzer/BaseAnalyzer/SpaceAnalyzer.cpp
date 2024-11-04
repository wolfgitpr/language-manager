#include "SpaceAnalyzer.h"

namespace LangMgr {
    bool SpaceAnalyzer::contains(const QChar &c) const {
        return c == ' ';
    }

    QString SpaceAnalyzer::randString() const {
        return " ";
    }

} // LangMgr