#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/PhonemeDict.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Task/Task.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::TemplateG2p::Internal::V2
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

    TemplateG2pTaskImpl::TemplateG2pTaskImpl(const LangCore::ModuleSpec *spec) : m_spec(spec) {}

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

            // 检查 LstmG2p 的 level，确保是 2（TemplateG2p V2 只支持 LstmG2p V2）
            auto g2pTask = g2pObj.as<LangCore::Task>();
            if (g2pTask->apiLevel() != 2) {
                return LangCore::Error(LangCore::Error::RuntimeError,
                                       stdc::formatN("TemplateG2p V2 only supports LstmG2p V2, but LstmG2p level is %1",
                                                     g2pTask->apiLevel()));
            }
            m_g2pInference = g2pTask;
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
        for (const auto &[lyric, mode, verifyError] : verifyRes) {
            LangCore::G2pErrorType wordErrorType = LangCore::NoError;
            if (!verifyError.empty()) {
                wordErrorType = LangCore::InvalidLyric;
            }
            res.emplace_back(LangCore::G2pRes{std::string(lyric), std::string(m_spec->name().text()), std::string(),
                                              std::vector<std::string>(), std::string(mode), wordErrorType});
        }

        // 第一遍处理：字典查找 + 收集需要批量转换的词
        std::vector<size_t> needBatchIndices; // 需要批量转换的词的索引
        std::vector<std::string> needBatchWords; // 需要批量转换的词

        for (size_t i = 0; i < res.size(); ++i) {
            auto &it = res[i];
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
                    // 字典查不到，收集起来批量处理
                    needBatchIndices.push_back(i);
                    needBatchWords.push_back(it.lyric);
                }
            } else {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    stdc::formatN(R"(Task "%1" - Fail: it.mode - "%2")", m_spec->name().text(), it.mode));
            }
        }

        // 批量处理字典查不到的词（50）
        constexpr int batchSize = 50;
        for (size_t batchStart = 0; batchStart < needBatchWords.size(); batchStart += batchSize) {
            size_t batchEnd = std::min(batchStart + batchSize, needBatchWords.size());
            size_t currentBatchSize = batchEnd - batchStart;

            // 创建批量输入
            auto batchInput = LangCore::NO<LangCore::G2pInputV1>::create();
            for (size_t i = batchStart; i < batchEnd; ++i) {
                batchInput->g2pInput.push_back(needBatchWords[i]);
            }

            // 调用 LstmG2p V2 进行批量转换
            if (!m_enableOnnxG2p || !m_g2pInference) {
                // 如果没有启用或没有初始化，使用原词
                for (size_t i = batchStart; i < batchEnd; ++i) {
                    size_t originalIndex = needBatchIndices[i];
                    res[originalIndex].pronunciation = res[originalIndex].lyric;
                    res[originalIndex].candidates = {res[originalIndex].lyric};
                }
                continue;
            }

            auto resultExp = m_g2pInference->start(batchInput);
            if (!resultExp) {
                // 批量转换失败，使用原词
                for (size_t i = batchStart; i < batchEnd; ++i) {
                    size_t originalIndex = needBatchIndices[i];
                    res[originalIndex].pronunciation = res[originalIndex].lyric;
                    res[originalIndex].candidates = {res[originalIndex].lyric};
                    res[originalIndex].errorType = LangCore::ModelInferenceFailed;
                }
                continue;
            }

            auto result = resultExp.take();
            if (const auto g2pResult = result.as<LangCore::G2pResultV1>()) {
                // 将批量结果映射回原始位置
                for (size_t i = 0; i < currentBatchSize; ++i) {
                    size_t originalIndex = needBatchIndices[batchStart + i];
                    if (i < g2pResult->g2pResult.size()) {
                        res[originalIndex].pronunciation = g2pResult->g2pResult[i].pronunciation;
                        // 如果 LSTM G2p 返回了错误类型，继承它
                        if (g2pResult->g2pResult[i].errorType != LangCore::NoError) {
                            res[originalIndex].errorType = g2pResult->g2pResult[i].errorType;
                        }
                    } else {
                        // 结果数量不匹配，使用原词
                        res[originalIndex].pronunciation = res[originalIndex].lyric;
                        res[originalIndex].candidates = {res[originalIndex].lyric};
                        res[originalIndex].errorType = LangCore::ModelInferenceFailed;
                    }
                }
            } else {
                // 返回结果类型错误
                if (!g2pResult->errorMessage.empty()) {
                    return LangCore::Error(
                        LangCore::Error::RuntimeError,
                        stdc::formatN(R"(Task "%1" - Fail: "%2")", m_spec->name().text(), g2pResult->errorMessage));
                }
                // 使用原词
                for (size_t i = batchStart; i < batchEnd; ++i) {
                    size_t originalIndex = needBatchIndices[i];
                    res[originalIndex].pronunciation = res[originalIndex].lyric;
                    res[originalIndex].candidates = {res[originalIndex].lyric};
                    res[originalIndex].errorType = LangCore::ModelInferenceFailed;
                }
            }
        }

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        g2pResult->g2pResult = res;

        return g2pResult;
    }

    std::string TemplateG2pTaskImpl::getConfig() const { return m_config; }

    LangCore::Expected<void> TemplateG2pTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }

} // namespace LangPlugins::TemplateG2p::Internal::V2
