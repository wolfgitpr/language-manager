#ifndef LANGCORE_LANGCOMMON_H
#define LANGCORE_LANGCOMMON_H

#include <string>
#include <utility>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

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
        std::string context;                    // Voice bank name (empty = default context)
        stdc::VersionNumber contextVersion;     // Voice bank version (isEmpty() = unversioned)

        G2pInput() = default;
        G2pInput(std::string lyric, std::string g2pId, std::string context = {},
                 stdc::VersionNumber contextVersion = {})
            : lyric(std::move(lyric)), g2pId(std::move(g2pId)), context(std::move(context)),
              contextVersion(std::move(contextVersion)) {}
    };

    enum G2pErrorType {
        NoError = 0,
        InvalidLyric,
        ModelInferenceFailed,
        PhonemeGenerationFailed,
        DriverUnavailable,
        UnknownError,
    };
    struct G2pRes {
        std::string lyric;
        std::string g2pId;
        std::string context;
        stdc::VersionNumber contextVersion;     // Voice bank version (isEmpty() = unversioned)
        std::string pronunciation;
        std::vector<std::string> candidates;
        std::string mode = "copy";
        G2pErrorType errorType = NoError;

        G2pRes() {}

        G2pRes(std::string lyric, std::string g2pId, std::string context = {},
               stdc::VersionNumber contextVersion = {}, std::string pronunciation = {},
               std::vector<std::string> candidates = {}, std::string mode = "copy",
               const G2pErrorType errorType = NoError) :
            lyric(std::move(lyric)), g2pId(std::move(g2pId)), context(std::move(context)),
            contextVersion(std::move(contextVersion)), pronunciation(std::move(pronunciation)),
            candidates(std::move(candidates)), mode(std::move(mode)), errorType(errorType) {
            if (this->candidates.empty() && !this->pronunciation.empty())
                this->candidates.push_back(this->pronunciation);

            if (this->pronunciation.empty())
                this->pronunciation = this->lyric;
        }

        /// Legacy convenience constructor (no contextVersion)
        G2pRes(std::string lyric, std::string g2pId, std::string context, std::string pronunciation,
               std::vector<std::string> candidates = {}, std::string mode = "copy",
               const G2pErrorType errorType = NoError) :
            G2pRes(std::move(lyric), std::move(g2pId), std::move(context), {},
                   std::move(pronunciation), std::move(candidates), std::move(mode), errorType) {}
    };
} // namespace LangCore
#endif // LANGCORE_LANGCOMMON_H
