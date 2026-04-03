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

    enum G2pErrorType {
        // 无错误
        NoError = 0,

        // 初始化错误
        InitError = 1,
        ModelInitFailed = 2,
        SessionInitFailed = 3,

        // 配置错误
        ConfigError = 4,

        // 输入验证错误
        InvalidInput = 5,
        EmptyInput = 6,
        InvalidLyric = 7,
        UnsupportedCharacter = 8,

        // 资源错误
        ResourceError = 9,
        ModelNotFound = 10,
        DictNotFound = 11,
        VocabNotFound = 12,

        // 转换错误
        ConversionError = 13,
        PinyinConversionFailed = 14,
        ModelInferenceFailed = 15,
        PhonemeGenerationFailed = 16,

        // 依赖错误
        DependencyError = 17,

        // 运行时错误
        RuntimeError = 18,
        TensorError = 19,
        SessionError = 20,

        // 未知错误
        UnknownError = 21,
    };
    struct G2pRes {
        std::string lyric;
        std::string g2pId;
        std::string pronunciation;
        std::vector<std::string> candidates;
        std::string mode = "copy";
        G2pErrorType errorType = NoError;

        G2pRes() {}

        G2pRes(std::string lyric, std::string g2pId, std::string pronunciation = "",
               std::vector<std::string> candidates = {}, std::string mode = "copy",
               const G2pErrorType errorType = NoError) :
            lyric(std::move(lyric)), g2pId(std::move(g2pId)), pronunciation(std::move(pronunciation)),
            candidates(std::move(candidates)), mode(std::move(mode)), errorType(errorType) {
            if (this->candidates.empty() && !this->pronunciation.empty())
                this->candidates.push_back(this->pronunciation);

            if (this->pronunciation.empty())
                this->pronunciation = this->lyric;
        }
    };
} // namespace LangCore
#endif // LANGCORE_LANGCOMMON_H
