#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/PhonemeDict.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::TemplateG2p::Internal::V1
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

    TemplateG2pTaskImpl::TemplateG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> TemplateG2pTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        auto cfg = LangCore::config(m_spec);

        // Parse verify entries
        auto verifyEntryExp = parseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        auto verifyEntry = verifyEntryExp.take();

        // Optional fields with defaults
        m_enableDict = cfg.getBool("enableDict", false);
        m_enableOnnxG2p = cfg.getBool("enableOnnxG2p", false);

        // Required fields
        auto dictPathExp = cfg.getPath("dictPath");
        if (!dictPathExp) {
            return dictPathExp.takeError();
        }
        auto dictPath = dictPathExp.take();

        auto onnxG2pIdExp = cfg.getString("onnxG2pId");
        if (!onnxG2pIdExp) {
            return onnxG2pIdExp.takeError();
        }
        auto onnxG2pId = onnxG2pIdExp.take();

        if (!m_enableOnnxG2p) {
            m_g2pInference = nullptr;
        } else {
            // Get g2p from package manager
            auto g2pCate = m_spec->Mgr()->category("g2p");
            if (!g2pCate) {
                return LangCore::Error(LangCore::Error::RuntimeError, "could not find category: g2p");
            }

            auto g2pObj = g2pCate->getFirstObject(onnxG2pId);
            if (!g2pObj) {
                return LangCore::Error(LangCore::Error::RuntimeError, "could not find id: " + onnxG2pId);
            }
            m_g2pInference = g2pObj.as<LangCore::Task>();
        }

        // Load phoneme dict
        if (m_enableDict) {
            if (dictPath.empty())
                return LangCore::Error(LangCore::Error::FileSystemError,
                                       stdc::formatN("Task '%1' - No dictPath specified", m_spec->name().text()));
            if (std::error_code error_code; !m_phonemeDict.load(dictPath, &error_code))
                return LangCore::Error(LangCore::Error::FileSystemError,
                                       stdc::formatN("Task '%1' - Failed to read dictionary %2:%3",
                                                     m_spec->name().text(), dictPath, error_code.value()));
        }

        // Save configuration
        m_config = LangCore::JsonValue(cfg.raw()).toJson();

        return {};
    }

    std::vector<std::string> TemplateG2pTaskImpl::lookup(const std::string &key) const {
        if (const auto it = m_phonemeDict.find(key.c_str()); it != m_phonemeDict.end()) {
            const auto &phonemes = it->second;
            std::vector<std::string> tokens;
            for (const char *buf : phonemes) {
                tokens.emplace_back(buf);
            }
            return tokens;
        }
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateG2pTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        {
            std::shared_lock lock(m_mutex);
            if (!m_g2pInference && m_enableOnnxG2p)
                return LangCore::Error(LangCore::Error::RuntimeError, "TemplateG2pTask: g2p inference not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        // Parse verify entries
        auto cfg = LangCore::config(m_spec);
        auto verifyEntryExp = parseVerifyEntries(cfg.raw(), m_spec->path());
        if (!verifyEntryExp) {
            return verifyEntryExp.takeError();
        }
        VerifierHolder verifierHolder(verifyEntryExp.take());

        std::vector<LangCore::G2pRes> res;
        const auto verifyRes = verifierHolder.verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangCore::G2pRes{
                lyric, m_spec->name().text(), "", {}, mode});

        for (auto &it : res) {
            if (it.mode == "copy") {
                it.pronunciation = it.lyric;
                it.candidates = {it.pronunciation};
            } else if (it.mode == "convert") {
                if (const auto findResult = lookup(it.lyric); m_enableDict && !findResult.empty()) {
                    std::string pronStr;
                    for (auto &phone : findResult)
                        pronStr += phone + " ";
                    it.pronunciation = pronStr;
                } else {
                    const auto lstmInput = LangCore::NO<LangCore::G2pInputV1>::create();
                    lstmInput->g2pInput.push_back({it.lyric});

                    if (!m_enableOnnxG2p) {
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                        continue;
                    }

                    if (!m_g2pInference) {
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                        continue;
                    }

                    auto resultExp = m_g2pInference->start(lstmInput);
                    if (!resultExp)
                        return LangCore::Error(LangCore::Error::RuntimeError,
                                               stdc::formatN(R"(Task "%1" - LstmG2p inference failed: "%2")",
                                                             m_spec->name().text(), resultExp.error().message()));

                    auto result = resultExp.take();
                    if (const auto g2pResult = result.as<LangCore::G2pResultV1>()) {
                        it.pronunciation = g2pResult->g2pResult[0].pronunciation;
                    } else {
                        if (!g2pResult->errorMessage.empty()) {
                            return LangCore::Error(LangCore::Error::RuntimeError,
                                                   stdc::formatN(R"(Task "%1" - Fail: "%2")",
                                                                 m_spec->name().text(), g2pResult->errorMessage));
                        }
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                    }
                }
            } else
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    stdc::formatN(R"(Task "%1" - Fail: it.mode - "%2")", m_spec->name().text(), it.mode));
        }

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        g2pResult->g2pResult = res;

        return g2pResult;
    }

    std::string TemplateG2pTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> TemplateG2pTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }

} // namespace LangPlugins::TemplateG2p::Internal::V1