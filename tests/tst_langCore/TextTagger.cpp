#include "TextTagger.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>

#include <re2/re2.h>

#include <LangCore/Support/JSON.h>

namespace TestUtils
{
    // ---- 内部 tagger 规则 ----

    struct TaggerEntry {
        std::string type; // "regex", "array", "dict"
        std::vector<std::string> value;
        std::string tag;
        bool discard = false;
    };

    class ITaggerRule {
    public:
        virtual ~ITaggerRule() = default;
        virtual void apply(std::vector<LangCore::TaggerRes> &input) = 0;
    };

    class RegexTaggerRule : public ITaggerRule {
    public:
        RegexTaggerRule(const std::string &language, const TaggerEntry &entry) : m_language(language), m_entry(entry) {
            RE2::Options options;
            options.set_encoding(RE2::Options::EncodingUTF8);
            options.set_log_errors(false);
            options.set_max_mem(8 << 20);

            // 合并多个模式
            std::string merged;
            for (size_t i = 0; i < entry.value.size(); ++i) {
                if (i > 0)
                    merged += "|";
                merged += entry.value[i];
            }
            m_regex = std::make_unique<RE2>(merged, options);
        }

        void apply(std::vector<LangCore::TaggerRes> &input) override {
            if (!m_regex || !m_regex->ok())
                return;
            for (auto &[lyric, language, tag, discard] : input) {
                if (language == "unknown" && RE2::FullMatch(lyric, *m_regex)) {
                    language = m_language;
                    tag = m_entry.tag;
                    discard = m_entry.discard;
                }
            }
        }

    private:
        std::string m_language;
        TaggerEntry m_entry;
        std::unique_ptr<RE2> m_regex;
    };

    class ArrayTaggerRule : public ITaggerRule {
    public:
        ArrayTaggerRule(const std::string &language, const TaggerEntry &entry)
            : m_language(language), m_entry(entry), m_set(entry.value.begin(), entry.value.end()) {}

        void apply(std::vector<LangCore::TaggerRes> &input) override {
            for (auto &[lyric, language, tag, discard] : input) {
                if (language == "unknown" && m_set.count(lyric)) {
                    language = m_language;
                    tag = m_entry.tag;
                    discard = m_entry.discard;
                }
            }
        }

    protected:
        std::string m_language;
        TaggerEntry m_entry;
        std::set<std::string> m_set;
    };

    class DictTaggerRule : public ArrayTaggerRule {
    public:
        DictTaggerRule(const std::string &language, const TaggerEntry &entry,
                       const std::vector<std::string> &resolvedPaths)
            : ArrayTaggerRule(language, entry) {
            // 加载所有字典文件
            for (const auto &path : resolvedPaths) {
                std::ifstream file(path);
                if (!file.is_open()) {
                    std::cerr << "Warning: Failed to open dictionary file: " << path << std::endl;
                    continue;
                }
                std::string line;
                while (std::getline(file, line)) {
                    if (line.empty())
                        continue;
                    if (const size_t tab_pos = line.find('\t'); tab_pos != std::string::npos) {
                        if (std::string word = line.substr(0, tab_pos); !word.empty())
                            m_set.insert(word);
                    }
                }
            }
        }
    };

    // ---- 全局状态 ----

    struct TaggerConfig {
        std::string language;
        std::vector<std::unique_ptr<ITaggerRule>> rules;
    };

    static std::vector<TaggerConfig> g_taggers;

    // 在 dictRootDir 下递归查找文件名匹配的文件
    static std::string findDictFile(const std::filesystem::path &dictRootDir, const std::string &filename) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dictRootDir)) {
            if (entry.is_regular_file() && entry.path().filename().string() == filename) {
                return entry.path().string();
            }
        }
        return {};
    }

    bool initTaggers(const std::filesystem::path &configDir, const std::filesystem::path &dictRootDir) {
        g_taggers.clear();

        for (const auto &entry : std::filesystem::directory_iterator(configDir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;

            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open tagger config: " << entry.path() << std::endl;
                return false;
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            std::string parseError;
            auto json = LangCore::JsonValue::fromJson(content, false, &parseError);
            if (!parseError.empty()) {
                std::cerr << "Failed to parse tagger config " << entry.path() << ": " << parseError << std::endl;
                return false;
            }

            const auto &obj = json.toObject();
            TaggerConfig cfg;

            // 读取 language
            auto langIt = obj.find("language");
            if (langIt == obj.end() || !langIt->second.isString()) {
                std::cerr << "Tagger config " << entry.path() << " missing 'language' field" << std::endl;
                return false;
            }
            cfg.language = langIt->second.toString();

            // 读取 tagger 规则数组
            auto taggerIt = obj.find("tagger");
            if (taggerIt == obj.end() || !taggerIt->second.isArray()) {
                std::cerr << "Tagger config " << entry.path() << " missing 'tagger' array" << std::endl;
                return false;
            }

            for (const auto &item : taggerIt->second.toArray()) {
                if (!item.isObject())
                    continue;

                const auto &itemObj = item.toObject();
                TaggerEntry te;

                // type
                auto typeIt = itemObj.find("type");
                if (typeIt == itemObj.end() || !typeIt->second.isString())
                    continue;
                te.type = typeIt->second.toString();

                // tag
                auto tagIt = itemObj.find("tag");
                if (tagIt == itemObj.end() || !tagIt->second.isString())
                    continue;
                te.tag = tagIt->second.toString();

                // value
                auto valueIt = itemObj.find("value");
                if (valueIt == itemObj.end() || !valueIt->second.isArray())
                    continue;
                for (const auto &v : valueIt->second.toArray()) {
                    if (v.isString())
                        te.value.push_back(v.toString());
                }

                // discard (optional, default false)
                auto discardIt = itemObj.find("discard");
                if (discardIt != itemObj.end() && discardIt->second.isBool())
                    te.discard = discardIt->second.toBool();

                // 创建规则
                if (te.type == "regex") {
                    cfg.rules.push_back(std::make_unique<RegexTaggerRule>(cfg.language, te));
                } else if (te.type == "array") {
                    cfg.rules.push_back(std::make_unique<ArrayTaggerRule>(cfg.language, te));
                } else if (te.type == "dict") {
                    // 解析字典路径
                    std::vector<std::string> resolvedPaths;
                    for (const auto &dictFile : te.value) {
                        auto resolved = findDictFile(dictRootDir, dictFile);
                        if (resolved.empty()) {
                            std::cerr << "Warning: Dictionary file not found: " << dictFile << std::endl;
                        } else {
                            resolvedPaths.push_back(resolved);
                        }
                    }
                    cfg.rules.push_back(std::make_unique<DictTaggerRule>(cfg.language, te, resolvedPaths));
                } else {
                    std::cerr << "Unknown tagger type: " << te.type << std::endl;
                }
            }

            g_taggers.push_back(std::move(cfg));
        }

        std::cout << "Loaded " << g_taggers.size() << " tagger configs" << std::endl;
        return true;
    }

    std::vector<LangCore::TaggerRes> tag(const std::vector<std::string> &input, bool discard,
                                          const std::vector<std::string> &priorityLanguages) {
        if (input.empty())
            return {};

        // 构建 TaggerRes
        std::vector<LangCore::TaggerRes> result;
        result.reserve(input.size());
        for (const auto &lyric : input)
            result.emplace_back(lyric);

        // 按优先级排序 taggers
        std::vector<size_t> order;
        order.reserve(g_taggers.size());

        // 先添加优先语言对应的 tagger
        std::set<size_t> added;
        for (const auto &lang : priorityLanguages) {
            for (size_t i = 0; i < g_taggers.size(); ++i) {
                if (g_taggers[i].language == lang && added.find(i) == added.end()) {
                    order.push_back(i);
                    added.insert(i);
                }
            }
        }
        // 再添加剩余的
        for (size_t i = 0; i < g_taggers.size(); ++i) {
            if (added.find(i) == added.end())
                order.push_back(i);
        }

        // 按顺序执行
        for (const auto idx : order) {
            for (const auto &rule : g_taggers[idx].rules) {
                rule->apply(result);
            }
        }

        // 过滤 discard
        if (discard) {
            result.erase(
                std::remove_if(result.begin(), result.end(),
                               [](const LangCore::TaggerRes &r) { return r.discard; }),
                result.end());
        }

        return result;
    }

} // namespace TestUtils
