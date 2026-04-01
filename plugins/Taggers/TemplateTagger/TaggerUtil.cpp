#include "TaggerUtil.h"

#include <fstream>
#include <sstream>

#include <LangCore/Support/Expected.h>
#include <stdcorelib/path.h>

namespace LangPlugins::TemplateTagger::V1
{
    ITaggerUtil::ITaggerUtil(TaggerUtilEntry entry, std::string language) :
        m_language(std::move(language)), m_entry(std::move(entry)) {}

    ITaggerUtil::~ITaggerUtil() = default;

    TaggerRegex::TaggerRegex(const TaggerUtilEntry &entry, const std::string &language) : ITaggerUtil(entry, language) {
        RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        RegexOptions.set_log_errors(true);
        RegexOptions.set_max_mem(8 << 20); // 8MB
    }

    TaggerRegex::~TaggerRegex() = default;

    LangCore::Expected<void> TaggerRegex::init() {
        regex_ = std::make_unique<RE2>(mergePatterns(m_entry.value), RegexOptions);
        if (!regex_->ok()) {
            return LangCore::Error(LangCore::Error::ConfigError, "Invalid regex pattern: " + regex_->error());
        }
        return {};
    }

    void TaggerRegex::tagger(std::vector<LangCore::TaggerRes> &input) {
        for (auto &[lyric, language, tag, discard] : input) {
            if (language == "unknown" && RE2::FullMatch(lyric, *regex_)) {
                language = m_language;
                tag = m_entry.tag;
                discard = m_entry.discard;
            }
        }
    }

    std::string TaggerRegex::mergePatterns(const std::vector<std::string> &patterns) {
        if (patterns.empty())
            return "";
        std::ostringstream oss;
        oss << patterns[0];
        for (size_t i = 1; i < patterns.size(); ++i)
            oss << "|" << patterns[i];
        return oss.str();
    }

    TaggerArray::TaggerArray(const TaggerUtilEntry &entry, const std::string &language) :
        ITaggerUtil(entry, language) {}

    TaggerArray::~TaggerArray() = default;

    LangCore::Expected<void> TaggerArray::init() {
        array = std::set(m_entry.value.begin(), m_entry.value.end());
        return {};
    }

    void TaggerArray::tagger(std::vector<LangCore::TaggerRes> &input) {
        for (auto &[lyric, language, tag, discard] : input) {
            if (language == "unknown" && array.find(lyric) != array.end()) {
                language = m_language;
                tag = m_entry.tag;
                discard = m_entry.discard;
            }
        }
    }

    TaggerDict::TaggerDict(const TaggerUtilEntry &entry, const std::string &language) : TaggerArray(entry, language) {}

    TaggerDict::~TaggerDict() = default;

    LangCore::Expected<void> TaggerDict::init() {
        auto wordsExp = loadWordsFromTxtFiles({m_entry.value.rbegin(), m_entry.value.rend()});
        if (!wordsExp) {
            return wordsExp.takeError();
        }
        array = wordsExp.take();
        return {};
    }

    LangCore::Expected<std::set<std::string>> TaggerDict::loadWordsFromTxtFiles(const std::vector<std::string> &paths) {
        std::set<std::string> words;

        for (const auto &path : paths) {
            if (!std::filesystem::exists(path)) {
                return LangCore::Error(LangCore::Error::ConfigError, "Dictionary file not found: " + path);
            }

            std::ifstream file(path);
            if (!file.is_open()) {
                return LangCore::Error(LangCore::Error::ConfigError, "Failed to open dictionary file: " + path);
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty())
                    continue;
                if (const size_t tab_pos = line.find('\t'); tab_pos != std::string::npos) {
                    if (std::string word = line.substr(0, tab_pos); !word.empty())
                        words.insert(word);
                }
            }
            file.close();
        }
        return words;
    }

    LangCore::Expected<std::unique_ptr<TaggerUtil>> TaggerUtil::Create(const std::vector<TaggerUtilEntry> &entries,
                                                                       const std::string &language) {
        auto taggerUtil = std::unique_ptr<TaggerUtil>(new TaggerUtil());

        for (const auto &entry : entries) {
            std::unique_ptr<ITaggerUtil> util;
            if (entry.type == "regex") {
                util = std::make_unique<TaggerRegex>(entry, language);
            } else if (entry.type == "array") {
                util = std::make_unique<TaggerArray>(entry, language);
            } else if (entry.type == "dict") {
                util = std::make_unique<TaggerDict>(entry, language);
            } else {
                return LangCore::Error(LangCore::Error::ConfigError, "Unknown tagger util type: " + entry.type);
            }

            if (auto initExp = util->init(); !initExp) {
                return initExp.takeError();
            }

            taggerUtil->m_taggerUtils.push_back(std::move(util));
        }

        return taggerUtil;
    }

    void TaggerUtil::tagger(std::vector<LangCore::TaggerRes> &input) const {
        for (const auto &util : m_taggerUtils)
            util->tagger(input);
    }

} // namespace LangPlugins::TemplateTagger::V1
