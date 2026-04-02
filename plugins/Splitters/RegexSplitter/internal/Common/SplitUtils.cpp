#include "SplitUtils.h"
#include <re2/re2.h>
#include <LangCore/Support/Error.h>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    std::vector<std::string> SplitUtils::splitWithPattern(
        const std::string &text,
        const std::string &pattern
    ) {
        std::vector<std::string> result;

        RE2::Options options;
        options.set_encoding(RE2::Options::EncodingUTF8);
        options.set_log_errors(false);
        options.set_max_mem(8 << 20); // 8MB

        RE2 regex(pattern, options);
        if (!regex.ok()) {
            // 如果正则表达式无效，返回原始文本
            result.push_back(text);
            return result;
        }

        re2::StringPiece textPiece(text);
        re2::StringPiece match;
        size_t last_end = 0;

        while (RE2::FindAndConsume(&textPiece, regex, &match)) {
            if (match.empty()) {
                result.emplace_back(text);
                return result;
            }

            const size_t match_start = match.data() - text.data();

            if (match_start > last_end) {
                result.emplace_back(text.data() + last_end, match_start - last_end);
            }

            result.emplace_back(match.data(), match.size());
            last_end = match_start + match.size();
        }

        if (last_end < text.size()) {
            result.emplace_back(text.data() + last_end, text.size() - last_end);
        }

        if (result.empty()) {
            result.emplace_back(text);
        }

        return result;
    }

    std::vector<std::string> SplitUtils::splitWithPatterns(
        const std::string &text,
        const std::vector<std::string> &patterns,
        bool caseSensitive
    ) {
        if (patterns.empty()) {
            return {text};
        }

        std::vector<std::string> result = {text};

        RE2::Options options;
        options.set_encoding(RE2::Options::EncodingUTF8);
        options.set_log_errors(false);
        options.set_max_mem(8 << 20); // 8MB
        options.set_case_sensitive(caseSensitive);

        for (const auto &pattern : patterns) {
            std::vector<std::string> newResult;
            newResult.reserve(result.size() * 2);

            for (const auto &segment : result) {
                RE2 regex(pattern, options);
                if (!regex.ok()) {
                    newResult.push_back(segment);
                    continue;
                }

                re2::StringPiece textPiece(segment);
                re2::StringPiece match;
                size_t last_end = 0;

                while (RE2::FindAndConsume(&textPiece, regex, &match)) {
                    if (match.empty()) {
                        newResult.push_back(segment);
                        break;
                    }

                    const size_t match_start = match.data() - segment.data();

                    if (match_start > last_end) {
                        newResult.emplace_back(segment.data() + last_end, match_start - last_end);
                    }

                    newResult.emplace_back(match.data(), match.size());
                    last_end = match_start + match.size();
                }

                if (last_end < segment.size()) {
                    newResult.emplace_back(segment.data() + last_end, segment.size() - last_end);
                }

                if (newResult.empty()) {
                    newResult.push_back(segment);
                }
            }

            result = std::move(newResult);
        }

        return result;
    }

    LangCore::Expected<void> SplitUtils::validateRegex(const std::string &pattern) {
        RE2::Options options;
        options.set_encoding(RE2::Options::EncodingUTF8);
        options.set_log_errors(false);

        RE2 regex(pattern, options);
        if (!regex.ok()) {
            return LangCore::Error(
                LangCore::Error::ConfigError,
                "Invalid regex pattern: " + regex.error(),
                "Check the regex pattern syntax"
            );
        }

        return {};
    }

    LangCore::Expected<std::vector<std::string>> SplitUtils::parsePatterns(
        const std::string &config
    ) {
        // 简化实现：直接返回空
        // 实际实现应该解析 JSON 配置并返回 patterns
        return std::vector<std::string>();
    }
} // namespace LangPlugins::RegexSplitter::Internal::Common