#ifndef LANGCORE_LANGCOMMON_H
#define LANGCORE_LANGCOMMON_H

#include <string>
#include <utility>
#include <vector>

namespace LangCore
{
    struct TaggerRes {
        std::string lyric;
        std::string language = "unknown";
        std::string tag = "unknown";
        bool discard = false;

        explicit TaggerRes(std::string lyric) : lyric(std::move(lyric)) {}
        explicit TaggerRes(std::string lyric, std::string language, std::string tag) :
            lyric(std::move(lyric)), language(std::move(language)), tag(std::move(tag)) {}
    };

    struct G2pInput {
        std::string lyric;
        std::string g2pId;
        G2pInput(std::string lyric, std::string g2pId) : lyric(std::move(lyric)), g2pId(std::move(g2pId)) {}
    };

    struct G2pRes {
        std::string lyric;
        std::string g2pId;
        std::string pronunciation = lyric;
        std::vector<std::string> candidates = {pronunciation};
        std::string mode = "copy";
        bool error = true;

        explicit G2pRes(std::string lyric, std::string g2pId, std::string pronunciation = "",
                        std::vector<std::string> candidates = {}, std::string mode = "copy", const bool error = true) :
            lyric(std::move(lyric)), g2pId(std::move(g2pId)), pronunciation(std::move(pronunciation)),
            candidates(std::move(candidates)), mode(std::move(mode)), error{error} {}
    };
} // namespace LangCore
#endif // LANGCORE_LANGCOMMON_H
