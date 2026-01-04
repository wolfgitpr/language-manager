#ifndef LANGMGR_COMMONAPIL1_H
#define LANGMGR_COMMONAPIL1_H

#include <string>
#include <vector>

namespace LangPlugins::Api::Common::L1
{
    struct TaggerRes {
        std::string lyric;
        std::string language = "unknown";
        std::string tag = "unknown";

        explicit TaggerRes(std::string lyric, std::string language, std::string tag) :
            lyric(std::move(lyric)), language(std::move(language)), tag(std::move(tag)) {}
    };

    struct G2pInput {
        std::string lyric;
        std::string g2pId;
    };

    struct G2pRes {
        std::string lyric;
        std::string g2pId = "unknown";
        std::string pronunciation = lyric;
        std::vector<std::string> candidates = {pronunciation};
        std::string mode = "copy";
        bool error = true;

        G2pRes() {}
        explicit G2pRes(std::string lyric, std::string g2pId, std::string pronunciation,
                        std::vector<std::string> candidates = {}, const std::string &mode = "copy",
                        const bool error = true) :
            lyric(std::move(lyric)), g2pId(std::move(g2pId)), pronunciation(std::move(pronunciation)),
            candidates(std::move(candidates)), mode(mode), error{error} {}
    };

} // namespace LangPlugins::Api::Common::L1
#endif // LANGMGR_COMMONAPIL1_H
