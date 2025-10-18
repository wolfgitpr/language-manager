#ifndef LANGCOMMON_H
#define LANGCOMMON_H

#include <string>
#include <vector>

#include <LangMgr/LangMgrGlobal.h>


struct LANGMGR_EXPORT LangNote {
    std::u32string lyric;
    std::u32string syllable;
    std::u32string syllableRevised;
    std::vector<std::u32string> candidates;
    std::string standardG2pId = "unknown";
    std::string g2pId = "unknown";
    std::string language = "unknown";
    bool revised = false;
    bool error = false;

    LangNote() = default;

    explicit LangNote(std::u32string lyric) : lyric(std::move(lyric)) {}
};
#endif // LANGCOMMON_H
