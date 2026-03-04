#include "TaggerUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "LangCore/Task/Task.h"

namespace LangPlugins::TemplateTagger
{

    ITaggerUtil::ITaggerUtil(Api::TemplateTagger::L1::TaggerUtilEntry entry, std::string language) :
        m_language(std::move(language)), m_entry(std::move(entry)) {}

    ITaggerUtil::~ITaggerUtil() = default;

    TaggerRegex::TaggerRegex(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language) :
        ITaggerUtil(entry, language) {
        RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        RegexOptions.set_log_errors(true);
        RegexOptions.set_max_mem(8 << 20); // 8MB

        regex_ = std::make_unique<RE2>(mergePatterns(m_entry.value), RegexOptions);
        if (!regex_->ok())
            throw std::runtime_error("Invalid regex: " + regex_->error());
    }

    TaggerRegex::~TaggerRegex() = default;

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
        oss << "(?:" << patterns[0] << ")";

        for (size_t i = 1; i < patterns.size(); ++i)
            oss << "|(?:" << patterns[i] << ")";

        return oss.str();
    }

    TaggerArray::TaggerArray(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language) :
        ITaggerUtil(entry, language) {
        array = std::set<std::string>({m_entry.value.begin(), m_entry.value.end()});
    }

    TaggerArray::~TaggerArray() = default;

    void TaggerArray::tagger(std::vector<LangCore::TaggerRes> &input) {
        for (auto &[lyric, language, tag, discard] : input) {
            if (language == "unknown" && array.find(lyric) != array.end()) {
                language = m_language;
                tag = m_entry.tag;
                discard = m_entry.discard;
            }
        }
    }

    TaggerDict::TaggerDict(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language) :
        TaggerArray(entry, language) {
        array = loadWordsFromTxtFiles({m_entry.value.rbegin(), m_entry.value.rend()});
    }

    TaggerDict::~TaggerDict() = default;

    std::set<std::string> TaggerDict::loadWordsFromTxtFiles(const std::vector<std::string> &paths) {
        std::set<std::string> words;

        for (const auto &path : paths) {
            if (!std::filesystem::exists(path)) {
                std::cerr << "warning: file not exist - " << path << std::endl;
                continue;
            }

            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "warning: fail to open file - " << path << std::endl;
                continue;
            }

            std::string line;
            size_t line_number = 0;

            while (std::getline(file, line)) {
                line_number++;

                if (line.empty())
                    continue;

                if (const size_t tab_pos = line.find('\t'); tab_pos != std::string::npos) {
                    if (std::string word = line.substr(0, tab_pos); !word.empty())
                        words.insert(word);
                }
            }
            file.close();
            std::cout << "from " << path << " load " << line_number << " lines" << std::endl;
        }
        return words;
    }

    TaggerUtil::TaggerUtil(const std::vector<Api::TemplateTagger::L1::TaggerUtilEntry> &entries, std::string language) {
        for (const auto &entry : entries) {
            if (entry.type == "regex")
                m_taggerUtils.emplace_back(std::make_unique<TaggerRegex>(entry, language));
            else if (entry.type == "array")
                m_taggerUtils.emplace_back(std::make_unique<TaggerArray>(entry, language));
            else if (entry.type == "dict")
                m_taggerUtils.emplace_back(std::make_unique<TaggerDict>(entry, language));
            else
                throw std::errc::invalid_argument;
        }
    }

    void TaggerUtil::tagger(std::vector<LangCore::TaggerRes> &input) const {
        for (const auto &taggerUtil : m_taggerUtils)
            taggerUtil->tagger(input);
    }

} // namespace LangPlugins::TemplateTagger
