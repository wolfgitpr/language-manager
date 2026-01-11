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

} // namespace LangPlugins::Api::Common::L1
#endif // LANGMGR_COMMONAPIL1_H
