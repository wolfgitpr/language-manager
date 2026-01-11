#ifndef LANGCOMMON_H
#define LANGCOMMON_H

#include <string>
#include <vector>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{
    struct LANGMGR_EXPORT TaggerRes {
        std::string lyric;
        std::string language = "unknown";
        std::string tag = "unknown";

        explicit TaggerRes(std::string lyric, std::string language, std::string tag) :
            lyric(std::move(lyric)), language(std::move(language)), tag(std::move(tag)) {}
    };

    struct LANGMGR_EXPORT G2pInput {
        std::string lyric;
        std::string g2pId;
    };

    enum g2pMode { Convert, Copy };
    struct LANGMGR_EXPORT G2pRes {
        std::string lyric;
        std::string g2pId = "unknown";
        std::string pronunciation = lyric;
        std::vector<std::string> candidates = {pronunciation};
        std::string mode = "copy";
        bool error = true;

        explicit G2pRes(std::string lyric, std::string g2pId, std::string pronunciation,
                        std::vector<std::string> candidates = {}, std::string mode = "copy", const bool error = true) :
            lyric(std::move(lyric)), g2pId(std::move(g2pId)), pronunciation(std::move(pronunciation)),
            candidates(std::move(candidates)), mode(std::move(mode)), error{error} {}
    };
} // namespace LangMgr
#endif // LANGCOMMON_H
