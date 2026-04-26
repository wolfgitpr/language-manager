#include "TextSplitter.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <re2/re2.h>

#include <LangCore/Support/JSON.h>

namespace TestUtils
{
    // 每个 splitter 配置：一组预编译正则模式
    struct SplitterConfig {
        std::string name;
        std::vector<std::unique_ptr<RE2>> regexes;
    };

    static std::vector<SplitterConfig> g_splitters;

    static std::vector<std::string> splitWithPattern(const std::string &text, const RE2 &regex) {
        std::vector<std::string> result;

        if (!regex.ok()) {
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

    bool initSplitters(const std::filesystem::path &configDir) {
        g_splitters.clear();

        for (const auto &entry : std::filesystem::directory_iterator(configDir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;

            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open splitter config: " << entry.path() << std::endl;
                return false;
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            std::string parseError;
            auto json = LangCore::JsonValue::fromJson(content, false, &parseError);
            if (!parseError.empty()) {
                std::cerr << "Failed to parse splitter config " << entry.path() << ": " << parseError << std::endl;
                return false;
            }

            SplitterConfig cfg;
            cfg.name = entry.path().stem().string();

            const auto &obj = json.toObject();
            auto regexesIt = obj.find("regexes");
            if (regexesIt != obj.end() && regexesIt->second.isArray()) {
                for (const auto &item : regexesIt->second.toArray()) {
                    if (item.isString()) {
                        RE2::Options options;
                        options.set_encoding(RE2::Options::EncodingUTF8);
                        options.set_log_errors(false);
                        options.set_max_mem(8 << 20);
                        cfg.regexes.push_back(std::make_unique<RE2>(item.toString(), options));
                    }
                }
            }

            if (!cfg.regexes.empty()) {
                g_splitters.push_back(std::move(cfg));
            }
        }

        std::cout << "Loaded " << g_splitters.size() << " splitter configs" << std::endl;
        return true;
    }

    std::vector<std::string> split(const std::string &input) {
        if (input.empty())
            return {};
        return split(std::vector<std::string>{input});
    }

    std::vector<std::string> split(const std::vector<std::string> &input) {
        if (input.empty())
            return {};

        std::vector<std::string> current = input;

        for (const auto &splitter : g_splitters) {
            for (const auto &regex : splitter.regexes) {
                std::vector<std::string> next;
                next.reserve(current.size() * 2);

                for (const auto &segment : current) {
                    auto parts = splitWithPattern(segment, *regex);
                    next.insert(next.end(), parts.begin(), parts.end());
                }

                current = std::move(next);
            }
        }

        return current;
    }

} // namespace TestUtils
