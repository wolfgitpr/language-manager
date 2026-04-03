#include "ModelStep.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include <LangCore/Core/PackageManager.h>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> ModelStep::configure(const LangCore::ModuleSpec *spec,
                                                   const LangCore::JsonObject &config)
    {
        m_spec = spec;

        // 解析 enabled
        auto enabledIt = config.find("enabled");
        if (enabledIt != config.end() && enabledIt->second.isBool()) {
            m_enabled = enabledIt->second.toBool();
        } else {
            m_enabled = true;
        }

        if (!m_enabled) {
            return {};
        }

        // 解析 onnxG2pId - 从传入的 config 中读取
        auto idIt = config.find("id");
        if (idIt == config.end() || !idIt->second.isString()) {
            return LangCore::Error(LangCore::Error::ConfigError, "Missing required field: id");
        }
        m_onnxG2pId = idIt->second.toString();

        // 解析 batchSize
        auto batchSizeIt = config.find("batchSize");
        if (batchSizeIt != config.end() && batchSizeIt->second.isInt()) {
            m_batchSize = static_cast<int>(batchSizeIt->second.toInt());
        } else {
            m_batchSize = 50;
        }

        // 获取 G2p 任务
        auto g2pCate = spec->Mgr()->category("g2p");
        if (!g2pCate) {
            return LangCore::Error(LangCore::Error::RuntimeError, "Could not find category: g2p");
        }

        auto g2pObj = g2pCate->getFirstObject(m_onnxG2pId);
        if (!g2pObj) {
            return LangCore::Error(LangCore::Error::RuntimeError,
                                 "Could not find g2p task: " + m_onnxG2pId);
        }

        m_onnxTask = g2pObj.as<LangCore::Task>();

        return {};
    }

    void ModelStep::handle(G2pContext &context)
    {
        if (!m_enabled || !m_onnxTask) {
            return;
        }

        // 收集需要模型推理的词
        std::vector<size_t> needInferenceIndices;
        std::vector<std::string> needInferenceWords;

        for (size_t i = 0; i < context.words().size(); ++i) {
            auto &word = context.words()[i];

            // 只处理需要转换、未丢弃、且字典查不到的词
            if (word.mode == "convert" && !word.discard && !word.fromDict &&
                word.pronunciation.empty()) {
                needInferenceIndices.push_back(i);
                // 使用清洗后的词（如果有）
                needInferenceWords.push_back(
                    word.cleanedLyric.empty() ? word.lyric : word.cleanedLyric
                );
            }
        }

        // 批量处理
        for (size_t batchStart = 0; batchStart < needInferenceWords.size(); batchStart += m_batchSize) {
            size_t batchEnd = std::min(batchStart + m_batchSize, needInferenceWords.size());

            std::vector<size_t> batchIndices(
                needInferenceIndices.begin() + batchStart,
                needInferenceIndices.begin() + batchEnd
            );
            std::vector<std::string> batchWords(
                needInferenceWords.begin() + batchStart,
                needInferenceWords.begin() + batchEnd
            );

            processBatch(context, batchIndices, batchWords);
        }
    }

    void ModelStep::processBatch(G2pContext &context,
                                  const std::vector<size_t> &indices,
                                  const std::vector<std::string> &words)
    {
        // 创建批量输入
        auto batchInput = LangCore::NO<LangCore::G2pInputV1>::create();
        for (const auto &word : words) {
            batchInput->g2pInput.push_back(word);
        }

        // 调用模型
        auto resultExp = m_onnxTask->start(batchInput);
        if (!resultExp) {
            // 批量转换失败，所有词使用原词
            for (size_t idx : indices) {
                auto &word = context.words()[idx];
                word.pronunciation = word.lyric;
                word.candidates = {word.lyric};
                word.errorType = LangCore::ModelInferenceFailed;
            }
            return;
        }

        auto result = resultExp.take();
        if (const auto g2pResult = result.as<LangCore::G2pResultV1>()) {
            // 将结果映射回原始位置
            for (size_t i = 0; i < indices.size(); ++i) {
                size_t originalIndex = indices[i];
                if (i < g2pResult->g2pResult.size()) {
                    auto &word = context.words()[originalIndex];
                    const auto &resultWord = g2pResult->g2pResult[i];

                    word.pronunciation = resultWord.pronunciation;
                    word.candidates = resultWord.candidates;
                    word.fromModel = true;
                    word.errorType = resultWord.errorType;
                } else {
                    // 结果数量不匹配
                    auto &word = context.words()[originalIndex];
                    word.pronunciation = word.lyric;
                    word.candidates = {word.lyric};
                    word.errorType = LangCore::ModelInferenceFailed;
                }
            }
        } else {
            // 返回结果类型错误
            for (size_t idx : indices) {
                auto &word = context.words()[idx];
                word.pronunciation = word.lyric;
                word.candidates = {word.lyric};
                word.errorType = LangCore::ModelInferenceFailed;
            }
        }
    }

    void ModelStep::cleanup()
    {
        m_onnxTask.reset();
    }

} // namespace LangPlugins::ChainG2p