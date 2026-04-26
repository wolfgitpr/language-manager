#include "TaskImpl.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Support/Tensor.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/TaskPlugin.h>

#include <InferUtil/TensorHelper.h>

#include <LangCore/Task/G2pTask.h>

namespace LangPlugins::LstmG2p::Internal::V1
{
    // Helper class for inference
    namespace InferenceHelper {
        static LangCore::Expected<LangCore::NO<LangCore::ITensor>>
        preprocessWord(const std::string &word, const std::map<std::string, int> &charVocab,
                       const int bosIdx, const int eosIdx, const int unkIdx) {
            std::string processedWord = stdc::to_lower(word);
            stdc::trim(processedWord);

            std::vector<int64_t> indices;
            indices.push_back(bosIdx); // BOS

            for (const char c : processedWord) {
                std::string charStr(1, c);
                if (auto it = charVocab.find(charStr); it != charVocab.end()) {
                    indices.push_back(it->second);
                } else {
                    indices.push_back(unkIdx);
                }
            }

            indices.push_back(eosIdx); // EOS

            const std::vector shape{static_cast<int64_t>(indices.size())};
            if (auto exp = LangCore::Tensor::createFromView<int64_t>(shape, stdc::array_view<int64_t>{indices}); exp) {
                return exp.take();
            }
            return LangCore::Error(LangCore::Error::ConfigError,
                                   stdc::formatN("Failed to create tensor for word: %1", word));
        }

        static LangCore::Expected<LangCore::NO<LangCore::ITensor>>
        getTensorFromResult(const LangCore::NO<LangCore::SessionResult> &result, const std::string &name) {
            const auto it = result->outputs.find(name);
            if (it == result->outputs.end()) {
                return LangCore::Error(LangCore::Error::RuntimeError,
                                       stdc::formatN("output '%1' not found in session result", name));
            }
            return it->second;
        }

        static LangCore::Expected<std::vector<int64_t>> runDecoder(
            const LangCore::NO<LangCore::SessionTask> &decodeSession,
            const LangCore::NO<LangCore::ITensor> &encoderOutputs,
            const LangCore::NO<LangCore::ITensor> &hidden,
            const LangCore::NO<LangCore::ITensor> &cell,
            int maxLen, int bosIdx, int eosIdx) {
            std::vector<int64_t> phonemeIds;
            const int64_t maxLen_ = maxLen > 0 ? maxLen : 48;

            // Initialize decoder input with BOS
            std::vector<int64_t> decoderInitData{bosIdx};
            std::vector<int64_t> decoderInitShape{1};
            LangCore::NO<LangCore::ITensor> decoderInput;

            if (auto exp = LangCore::Tensor::createFromView<int64_t>(decoderInitShape, stdc::array_view<int64_t>{decoderInitData}); exp) {
                decoderInput = exp.take();
            } else {
                return exp.takeError();
            }

            auto currentHidden = hidden;
            auto currentCell = cell;

            for (int64_t i = 0; i < maxLen_; ++i) {
                auto decoderSessionInput = LangCore::NO<LangCore::SessionStartInput>::create();
                decoderSessionInput->inputs["decoder_input"] = decoderInput;
                decoderSessionInput->inputs["hidden"] = currentHidden;
                decoderSessionInput->inputs["cell"] = currentCell;
                decoderSessionInput->inputs["encoder_outputs"] = encoderOutputs;

                decoderSessionInput->outputs.insert("output");
                decoderSessionInput->outputs.insert("hidden_new");
                decoderSessionInput->outputs.insert("cell_new");
                decoderSessionInput->outputs.insert("attention_weights");

                LangCore::NO<LangCore::SessionResult> decoderResult;
                if (auto decoderExp = decodeSession->start(decoderSessionInput); !decoderExp) {
                    return decoderExp.takeError();
                } else {
                    auto sessionTaskResult = decoderExp.take();
                    if (!sessionTaskResult) {
                        return LangCore::Error(LangCore::Error::RuntimeError, "invalid decoder result");
                    }
                    decoderResult = sessionTaskResult.as<LangCore::SessionResult>();
                }

                auto output = getTensorFromResult(decoderResult, "output");
                currentHidden = getTensorFromResult(decoderResult, "hidden_new").take();
                currentCell = getTensorFromResult(decoderResult, "cell_new").take();

                if (!output || !currentHidden || !currentCell) {
                    return LangCore::Error(LangCore::Error::RuntimeError, "failed to get decoder outputs");
                }

                // Get predicted phoneme ID (argmax)
                const auto outputTensor = output.take();
                if (outputTensor->dataType() != LangCore::ITensor::Float) {
                    return LangCore::Error(LangCore::Error::RuntimeError, "decoder output is not float");
                }

                auto outputView = outputTensor->view<float>();
                if (outputView.empty()) {
                    return LangCore::Error(LangCore::Error::RuntimeError, "decoder output is empty");
                }

                int64_t predictedId = 0;
                float maxProb = outputView[0];
                for (size_t j = 1; j < outputView.size(); ++j) {
                    if (outputView[j] > maxProb) {
                        maxProb = outputView[j];
                        predictedId = static_cast<int64_t>(j);
                    }
                }

                // Check for EOS
                if (predictedId == eosIdx) {
                    break;
                }

                phonemeIds.push_back(predictedId);

                // Update decoder input for next step
                std::vector nextInputData{predictedId};
                std::vector<int64_t> nextInputShape{1};
                if (auto exp = LangCore::Tensor::createFromView<int64_t>(nextInputShape, stdc::array_view<int64_t>{nextInputData}); exp) {
                    decoderInput = exp.take();
                } else {
                    return exp.takeError();
                }
            }

            return phonemeIds;
        }

        static LangCore::Expected<std::vector<std::string>> decodePhonemes(
            const std::vector<int64_t> &phonemeIds,
            const std::map<int, std::string> &idxToPhoneme,
            const int bosIdx, const int eosIdx, const int padIdx, const int unkIdx) {
            std::vector<std::string> phonemes;

            for (const int64_t id : phonemeIds) {
                // Skip special tokens
                if (id == bosIdx || id == eosIdx || id == padIdx || id == unkIdx) {
                    continue;
                }

                auto it = idxToPhoneme.find(static_cast<int>(id));
                if (it != idxToPhoneme.end()) {
                    phonemes.push_back(it->second);
                }
            }

            return phonemes;
        }
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    LstmG2pTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        if (g2pInput->g2pInput.empty())
            return LangCore::Error(LangCore::Error::ConfigError, "input words are empty");

        // Driver unavailable — graceful degradation
        if (!m_driverAvailable) {
            return makeFallbackResult(g2pInput->g2pInput);
        }

        {
            std::shared_lock lock(m_mutex);
            if (!m_driver)
                return LangCore::Error(LangCore::Error::RuntimeError, "inference driver not initialized");
        }

        // For now, process only the first word
        const auto &lyric = g2pInput->g2pInput[0];
        auto preprocessedInput = InferenceHelper::preprocessWord(lyric, m_charVocab, m_bosIdx, m_eosIdx, m_unkIdx);
        if (!preprocessedInput) {
            // 预处理失败，返回带错误类型的结果
            auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
            g2pResult->g2pResult = {LangCore::G2pRes{
                std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                LangCore::InvalidLyric}};
            return g2pResult;
        }

        // Run encoder
        auto encoderInput = LangCore::NO<LangCore::SessionStartInput>::create();
        encoderInput->inputs["input_ids"] = preprocessedInput.take();

        encoderInput->outputs.insert("encoder_outputs");
        encoderInput->outputs.insert("hidden");
        encoderInput->outputs.insert("cell");

        std::unique_lock lock(m_mutex);
        if (!m_encoderSession || !m_encoderSession->isOpen())
            return LangCore::Error(LangCore::Error::RuntimeError, "encoder session is not initialized");

        LangCore::NO<LangCore::SessionResult> encoderResult;
        if (auto encoderExp = m_encoderSession->start(encoderInput); !encoderExp) {
            // 编码器推理失败，返回带错误类型的结果
            auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
            g2pResult->g2pResult = {LangCore::G2pRes{
                std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                LangCore::ModelInferenceFailed}};
            return g2pResult;
        } else {
            auto sessionTaskResult = encoderExp.take();
            if (!sessionTaskResult) {
                auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
                g2pResult->g2pResult = {LangCore::G2pRes{
                    std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                    LangCore::ModelInferenceFailed}};
                return g2pResult;
            }
            encoderResult = sessionTaskResult.as<LangCore::SessionResult>();
        }

        // Extract encoder outputs
        auto encoderOutputs = InferenceHelper::getTensorFromResult(encoderResult, "encoder_outputs");
        auto hidden = InferenceHelper::getTensorFromResult(encoderResult, "hidden");
        auto cell = InferenceHelper::getTensorFromResult(encoderResult, "cell");

        if (!encoderOutputs || !hidden || !cell) {
            auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
            g2pResult->g2pResult = {LangCore::G2pRes{
                std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                LangCore::ModelInferenceFailed}};
            return g2pResult;
        }

        // Run decoder with autoregressive generation
        auto phonemeIds = InferenceHelper::runDecoder(m_decodeSession, encoderOutputs.take(), hidden.take(),
                                                      cell.take(), m_maxLen, m_bosIdx, m_eosIdx);
        if (!phonemeIds) {
            auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
            g2pResult->g2pResult = {LangCore::G2pRes{
                std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                LangCore::ModelInferenceFailed}};
            return g2pResult;
        }

        // Decode phonemes
        auto phonemes = InferenceHelper::decodePhonemes(phonemeIds.take(), m_idxToPhoneme, m_bosIdx,
                                                         m_eosIdx, m_padIdx, m_unkIdx);
        if (phonemes->empty()) {
            auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
            g2pResult->g2pResult = {LangCore::G2pRes{
                std::string(lyric), std::string("eng"), std::string(lyric), std::vector<std::string>(), std::string("copy"),
                LangCore::PhonemeGenerationFailed}};
            return g2pResult;
        }

        auto phonemes_ = phonemes.take();

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        std::string pronStr;
        for (auto &phone : phonemes_)
            pronStr += phone + " ";
        g2pResult->g2pResult = {LangCore::G2pRes{
            std::string(lyric), std::string("eng"), std::string(pronStr), std::vector<std::string>(), std::string("copy")}};

        return g2pResult;
    }

} // namespace LangPlugins::LstmG2p::Internal::V1