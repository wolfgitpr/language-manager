#include "TaskImpl.h"

#include <fstream>
#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/TaskPlugin.h>

#include <InferUtil/TensorHelper.h>

#include <LangCore/Task/G2pTask.h>

namespace LangPlugins::LstmG2p::Internal::V1
{
    // Helper function to load phoneme mapping from JSON file
    LangCore::Expected<std::map<std::string, int>>
    LstmG2pTaskImpl::loadPhonemeMapping(const std::filesystem::path &path, const std::string &fieldName) {
        std::map<std::string, int> out;

        std::ifstream file(path);
        if (!file.is_open()) {
            return LangCore::Error(
                LangCore::Error::FileSystemError,
                stdc::formatN(R"(error loading "%1": %2 file not found)", fieldName, stdc::path::to_utf8(path)));
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        std::string buffer(size, '\0');
        file.seekg(0);
        file.read(buffer.data(), size);

        std::string errString;
        const auto j = LangCore::JsonValue::fromJson(buffer, true, &errString);
        if (!errString.empty()) {
            return LangCore::Error(LangCore::Error::ConfigError, errString);
        }

        if (!j.isObject()) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                   stdc::formatN(R"(error loading "%1": outer JSON is not an object)", fieldName));
        }

        const auto &obj = j.toObject();
        for (const auto &[key, value] : obj) {
            if (!value.isInt()) {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    stdc::formatN(R"(error loading "%1": value of key "%2" is not int)", fieldName, key));
            }
            out[key] = static_cast<int>(value.toInt());
        }

        return out;
    }

    LstmG2pTaskImpl::LstmG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> LstmG2pTaskImpl::initialize() {
        std::unique_lock lock(m_mutex);

        // Get driver from package manager
        auto driverCate = m_spec->Mgr()->category("driver");
        if (!driverCate) {
            return LangCore::Error(LangCore::Error::RuntimeError, "could not find category: driver");
        }

        auto driverObj = driverCate->getFirstObject("g2pOnnxDriver");
        if (!driverObj) {
            return LangCore::Error(LangCore::Error::RuntimeError, "could not find id: g2pOnnxDriver");
        }
        m_driver = driverObj.as<LangCore::SessionFactory>();

        auto cfg = LangCore::config(m_spec);

        // Required fields
        auto encoderExp = cfg.getPath("encoder");
        if (!encoderExp) {
            return encoderExp.takeError();
        }
        auto encoder = encoderExp.take();

        auto decoderExp = cfg.getPath("decoder");
        if (!decoderExp) {
            return decoderExp.takeError();
        }
        auto decoder = decoderExp.take();

        // Load charVocab
        auto charVocabPathExp = cfg.getPath("charVocab");
        if (!charVocabPathExp) {
            return charVocabPathExp.takeError();
        }
        auto charVocabMapping = loadPhonemeMapping(charVocabPathExp.take(), "charVocab");
        if (!charVocabMapping) {
            return charVocabMapping.takeError();
        }
        m_charVocab = charVocabMapping.take();

        // Load phonemeVocab
        auto phonemeVocabPathExp = cfg.getPath("phonemeVocab");
        if (!phonemeVocabPathExp) {
            return phonemeVocabPathExp.takeError();
        }
        auto phonemeVocabMapping = loadPhonemeMapping(phonemeVocabPathExp.take(), "phonemeVocab");
        if (!phonemeVocabMapping) {
            return phonemeVocabMapping.takeError();
        }
        m_phonemeVocab = phonemeVocabMapping.take();

        for (const auto &[phoneme, index] : m_phonemeVocab)
            m_idxToPhoneme[index] = phoneme;

        m_encoderSession = m_driver->createSession();
        const auto encoderOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        encoderOpenArgs->useCpu = false;
        if (auto res = m_encoderSession->open(encoder, encoderOpenArgs); !res)
            return res;

        m_decodeSession = m_driver->createSession();
        const auto predictorOpenArgs = LangCore::NO<LangCore::SessionOpenArgs>::create();
        predictorOpenArgs->useCpu = false;
        if (auto res = m_decodeSession->open(decoder, predictorOpenArgs); !res)
            return res;

        // Save configuration
        m_config = LangCore::JsonValue(cfg.raw()).toJson();

        return {};
    }

    // Helper class for inference
    namespace InferenceHelper {
        static LangCore::Expected<LangCore::NO<LangCore::ITensor>>
        preprocessWord(const std::string &word, std::map<std::string, int> charVocab,
                       const int bosIdx, const int eosIdx, const int unkIdx) {
            const std::string processedWord = stdc::to_lower(word);
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
        {
            std::shared_lock lock(m_mutex);
            if (!m_driver)
                return LangCore::Error(LangCore::Error::RuntimeError, "inference driver not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");

        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        // Preprocess input word
        if (g2pInput->g2pInput.empty())
            return LangCore::Error(LangCore::Error::ConfigError, "input words are empty");

        // For now, process only the first word
        const auto &lyric = g2pInput->g2pInput[0];
        auto preprocessedInput = InferenceHelper::preprocessWord(lyric, m_charVocab, m_bosIdx, m_eosIdx, m_unkIdx);
        if (!preprocessedInput)
            return preprocessedInput.takeError();

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
            return encoderExp.takeError();
        } else {
            auto sessionTaskResult = encoderExp.take();
            if (!sessionTaskResult) {
                return LangCore::Error(LangCore::Error::RuntimeError, "invalid encoder result");
            }
            encoderResult = sessionTaskResult.as<LangCore::SessionResult>();
        }

        // Extract encoder outputs
        auto encoderOutputs = InferenceHelper::getTensorFromResult(encoderResult, "encoder_outputs");
        auto hidden = InferenceHelper::getTensorFromResult(encoderResult, "hidden");
        auto cell = InferenceHelper::getTensorFromResult(encoderResult, "cell");

        if (!encoderOutputs || !hidden || !cell)
            return LangCore::Error(LangCore::Error::RuntimeError, "failed to get encoder outputs");

        // Run decoder with autoregressive generation
        auto phonemeIds = InferenceHelper::runDecoder(m_decodeSession, encoderOutputs.take(), hidden.take(),
                                                      cell.take(), m_maxLen, m_bosIdx, m_eosIdx);
        if (!phonemeIds)
            return phonemeIds.takeError();

        // Decode phonemes
        auto phonemes = InferenceHelper::decodePhonemes(phonemeIds.take(), m_idxToPhoneme, m_bosIdx,
                                                         m_eosIdx, m_padIdx, m_unkIdx);
        if (phonemes->empty())
            return phonemes.takeError();

        auto phonemes_ = phonemes.take();

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        std::string pronStr;
        for (auto &phone : phonemes_)
            pronStr += phone + " ";
        g2pResult->g2pResult = {LangCore::G2pRes(lyric, "eng", pronStr, {}, "copy")};

        return g2pResult;
    }

    std::string LstmG2pTaskImpl::getConfig() const {
        return m_config;
    }

    LangCore::Expected<void> LstmG2pTaskImpl::setConfig(const std::string &config) {
        m_config = config;
        return {};
    }

} // namespace LangPlugins::LstmG2p::Internal::V1