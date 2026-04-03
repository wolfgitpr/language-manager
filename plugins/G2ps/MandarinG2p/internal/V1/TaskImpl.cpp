#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Support/ConfigAccessor.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Pinyin.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::MandarinG2p::Internal::V1
{
    // Helper function to parse verify entries from JSON
    static LangCore::Expected<std::vector<InferUtil::VerifyEntry>>
        parseVerifyEntries(const LangCore::JsonObject &config, const std::filesystem::path &basePath) {
        std::vector<InferUtil::VerifyEntry> entries;

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
            InferUtil::VerifyEntry entry;

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

    // Helper class to manage verifier
    class VerifierHolder {
    public:
        std::unique_ptr<InferUtil::Verifier> verifier;

        explicit VerifierHolder(const std::vector<InferUtil::VerifyEntry> &entries) {
            auto exp = InferUtil::Verifier::Create(entries);
            if (exp) {
                verifier = exp.take();
            }
        }

        std::vector<std::tuple<std::string, std::string, std::string>>
        verify(const std::vector<std::string> &input) const {
            if (verifier) {
                // Convert VerifyRes to tuple format
                auto verifyResults = verifier->verify(input);
                std::vector<std::tuple<std::string, std::string, std::string>> result;
                result.reserve(verifyResults.size());
                for (const auto &res : verifyResults) {
                    result.emplace_back(res.lyric, res.mode, res.error ? "error" : "");
                }
                return result;
            }
            return {};
        }
    };

    MandarinG2pTaskImpl::MandarinG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> MandarinG2pTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        m_result.reset();

        auto cfg = LangCore::config(m_spec);

        // Parse verify entries
        auto verifyEntryExp = parseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        auto verifyEntry = verifyEntryExp.take();

        // Required fields
        auto dictPathExp = cfg.getPath("dictPath");
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        auto dictPath = dictPathExp.take();

        Pinyin::setDictionaryPath(dictPath);
        m_mandarin = std::make_unique<Pinyin::Pinyin>();

        if (!m_mandarin->initialized())
            return {};

        return {};
    }

    static std::vector<std::vector<LangCore::G2pRes>> groupLyrics(const std::vector<LangCore::G2pRes> &input) {
        std::vector<std::vector<LangCore::G2pRes>> groups;
        std::string lastMode;

        for (const auto &item : input) {
            if (groups.empty() || item.mode != lastMode) {
                groups.emplace_back();
                lastMode = item.mode;
            }
            groups.back().push_back(item);
        }

        return groups;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    MandarinG2pTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        {
            std::shared_lock lock(m_mutex);
            if (!m_mandarin->initialized())
                return LangCore::Error(LangCore::Error::RuntimeError, "MandarinG2pTask: chinese g2p not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        std::vector<LangCore::G2pRes> res;
        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        // Parse verify entries
        auto cfg = LangCore::config(m_spec);
        auto verifyEntryExp = parseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        VerifierHolder verifierHolder(verifyEntryExp.take());

        const auto verifyRes = verifierHolder.verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, verifyError] : verifyRes) {
            LangCore::G2pErrorType wordErrorType = LangCore::NoError;
            if (!verifyError.empty()) {
                wordErrorType = LangCore::InvalidLyric;
            }
            res.emplace_back(LangCore::G2pRes{
                std::string(lyric), std::string(m_spec->name().text()), std::string(), std::vector<std::string>(), std::string(mode), wordErrorType});
        }

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            auto pinyinRes =
                m_mandarin->hanziToPinyin(_input, Pinyin::ManTone::NORMAL, Pinyin::Default, true, false, false);

            for (auto &[hanzi, pinyin, candidates, conversionError] : pinyinRes) {
                LangCore::G2pErrorType wordErrorType = LangCore::NoError;
                if (conversionError) {
                    wordErrorType = LangCore::PinyinConversionFailed;
                }
                LangCore::G2pRes newRes;
                newRes.lyric = std::string(hanzi);
                newRes.g2pId = std::string(m_spec->id());
                newRes.pronunciation = std::string(mode == "convert" ? pinyin : hanzi);
                newRes.candidates = std::vector<std::string>();
                newRes.mode = std::string(mode);
                newRes.errorType = wordErrorType;
                g2pResult->g2pResult.emplace_back(newRes);
            }
        }

        m_result = g2pResult;
        return g2pResult;
    }

    std::string MandarinG2pTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> MandarinG2pTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }

} // namespace LangPlugins::MandarinG2p::Internal::V1