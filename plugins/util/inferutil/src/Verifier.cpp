#include <inferutil/Verifier.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "LangCore/Task/Task.h"

namespace LangPlugins::inferUtil
{

    IVerify::IVerify(VerifyEntry entry) : entry_(std::move(entry)) {}

    IVerify::~IVerify() = default;

    VerifyRegex::VerifyRegex(const VerifyEntry &entry) : IVerify(entry) {
        RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
        RegexOptions.set_log_errors(true);
        RegexOptions.set_max_mem(8 << 20); // 8MB

        regex_ = std::make_unique<RE2>(mergePatterns(entry_.value), RegexOptions);
        if (!regex_->ok())
            throw std::runtime_error("Invalid regex: " + regex_->error());
    }

    VerifyRegex::~VerifyRegex() = default;

    void VerifyRegex::verify(std::vector<VerifyRes> &input) {
        for (auto &[lyric, mode, error] : input) {
            if (RE2::FullMatch(lyric, *regex_)) {
                mode = entry_.mode;
                error = false;
            }
        }
    }

    std::string VerifyRegex::mergePatterns(const std::vector<std::string> &patterns) {
        if (patterns.empty())
            return "";

        std::ostringstream oss;
        oss << "(?:" << patterns[0] << ")";

        for (size_t i = 1; i < patterns.size(); ++i)
            oss << "|(?:" << patterns[i] << ")";

        return oss.str();
    }

    VerifyArray::VerifyArray(const VerifyEntry &entry) : IVerify(entry) {
        array = std::set<std::string>({entry_.value.begin(), entry_.value.end()});
    }

    VerifyArray::~VerifyArray() = default;

    void VerifyArray::verify(std::vector<VerifyRes> &input) {
        for (auto &[lyric, mode, error] : input) {
            if (array.find(lyric) != array.end()) {
                mode = entry_.mode;
                error = false;
            }
        }
    }

    VerifyDict::VerifyDict(const VerifyEntry &entry) : VerifyArray(entry) {
        array = loadWordsFromTxtFiles({entry_.value.rbegin(), entry_.value.rend()});
    }

    VerifyDict::~VerifyDict() = default;

    std::set<std::string> VerifyDict::loadWordsFromTxtFiles(const std::vector<std::string> &paths) {
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
    Verifier::Verifier(const std::vector<VerifyEntry> &entries) {
        for (const auto &entry : entries) {
            if (entry.type == "regex")
                verifiers_.emplace_back(std::make_unique<VerifyRegex>(entry));
            else if (entry.type == "array")
                verifiers_.emplace_back(std::make_unique<VerifyArray>(entry));
            else if (entry.type == "dict")
                verifiers_.emplace_back(std::make_unique<VerifyDict>(entry));
            else
                throw std::errc::invalid_argument;
        }
    }

    std::vector<VerifyRes> Verifier::verify(const std::vector<std::string> &input) const {
        std::vector<VerifyRes> result;
        for (const auto lyric : input)
            result.emplace_back(VerifyRes{lyric, "copy", false});

        for (const auto &verifier : verifiers_)
            verifier->verify(result);
        return result;
    }

} // namespace LangPlugins::inferUtil
