#ifndef LANGMGR_VERIFIER_H
#define LANGMGR_VERIFIER_H

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>
#include <stdcorelib/path.h>
#include <stdcorelib/str.h>
#include <re2/re2.h>

namespace LangPlugins::InferUtil
{
    struct VerifyEntry {
        std::string type;
        std::vector<std::string> value;
        std::string mode;
    };

    struct VerifyRes {
        std::string lyric;
        std::string mode = "copy";
        bool error = true;
    };

    class IVerify {
    public:
        explicit IVerify(VerifyEntry entry);
        virtual ~IVerify();

        virtual LangCore::Expected<void> init() = 0;
        virtual void verify(std::vector<VerifyRes> &input) = 0;

    protected:
        VerifyEntry entry_;
    };

    class VerifyRegex : public IVerify {
    public:
        explicit VerifyRegex(const VerifyEntry &entry);
        ~VerifyRegex() override;

        LangCore::Expected<void> init() override;
        void verify(std::vector<VerifyRes> &input) override;

    private:
        RE2::Options regexOptions;
        std::unique_ptr<RE2> regex_;

        static std::string mergePatterns(const std::vector<std::string> &patterns);
    };

    class VerifyArray : public IVerify {
    public:
        explicit VerifyArray(const VerifyEntry &entry);
        ~VerifyArray() override;

        LangCore::Expected<void> init() override { return {}; }
        void verify(std::vector<VerifyRes> &input) override;

    protected:
        std::set<std::string> array;
    };

    class VerifyDict : public VerifyArray {
    public:
        explicit VerifyDict(const VerifyEntry &entry);
        ~VerifyDict() override;

        LangCore::Expected<void> init() override;

    private:
        static LangCore::Expected<std::set<std::string>> loadWordsFromTxtFiles(const std::vector<std::string> &paths);
    };

    class Verifier {
    public:
        static LangCore::Expected<std::unique_ptr<Verifier>> Create(const std::vector<VerifyEntry> &entries);
        ~Verifier() = default;

        std::vector<VerifyRes> verify(const std::vector<std::string> &input) const;

    private:
        Verifier() = default;
        std::vector<std::unique_ptr<IVerify>> verifiers_;
    };

    // Helper function to parse verify entries from JSON
    inline LangCore::Expected<std::vector<VerifyEntry>>
        ParseVerifyEntries(const LangCore::JsonObject &config, const std::filesystem::path &basePath) {
        std::vector<VerifyEntry> entries;

        const auto it = config.find("verify");
        if (it == config.end()) {
            return LangCore::Error(LangCore::Error::ConfigError, "verify field is missing");
        }

        if (!it->second.isArray()) {
            return LangCore::Error(LangCore::Error::ConfigError, "verify field must be an array");
        }

        const auto &arr = it->second.toArray();
        entries.reserve(arr.size());

        for (size_t i = 0; i < arr.size(); ++i) {
            const auto &item = arr[i];
            if (!item.isObject()) {
                return LangCore::Error(LangCore::Error::ConfigError,
                                       stdc::formatN("verify entry #%1 must be an object", i));
            }

            const auto &obj = item.toObject();
            VerifyEntry entry;

            if (const auto typeIt = obj.find("type"); typeIt != obj.end() && typeIt->second.isString()) {
                entry.type = typeIt->second.toString();
            } else {
                return LangCore::Error(LangCore::Error::ConfigError,
                                       stdc::formatN("verify entry #%1 missing or invalid 'type' field", i));
            }

            if (const auto valueIt = obj.find("value"); valueIt != obj.end() && valueIt->second.isArray()) {
                const auto &valueArr = valueIt->second.toArray();
                for (size_t j = 0; j < valueArr.size(); ++j) {
                    if (valueArr[j].isString()) {
                        if (entry.type == "dict") {
                            const auto path = basePath / stdc::path::from_utf8(valueArr[j].toString());
                            entry.value.push_back(path.string());
                        } else {
                            entry.value.push_back(valueArr[j].toString());
                        }
                    }
                }
            } else {
                return LangCore::Error(LangCore::Error::ConfigError,
                                       stdc::formatN("verify entry #%1 missing or invalid 'value' field", i));
            }

            if (const auto modeIt = obj.find("mode"); modeIt != obj.end() && modeIt->second.isString()) {
                entry.mode = modeIt->second.toString();
            } else {
                return LangCore::Error(LangCore::Error::ConfigError,
                                       stdc::formatN("verify entry #%1 missing or invalid 'mode' field", i));
            }

            entries.push_back(std::move(entry));
        }

        return entries;
    }

} // namespace LangPlugins::InferUtil

#endif // LANGMGR_VERIFIER_H
