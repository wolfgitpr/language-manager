#include "LstmG2pTask.h"

#include <mutex>
#include <numeric>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/G2pModule.h>
#include <LangCore/Task/Task.h>
#include <LangCore/Task/TaskFactoryPlugin.h>
#include <LangPlugins/Support/Tensor.h>

#include <inferutil/TensorHelper.h>

#include "LangCore/Task/G2pTask.h"

namespace LangPlugins::LstmG2p
{
    static LangCore::Expected<LangCore::NO<Lstm::LstmG2pConfiguration>> getConfig(const LangCore::G2pSpec *spec) {

        const auto genericConfig = spec->as<LangCore::G2pSpec>()->configuration();
        if (!genericConfig) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "LstmG2p configuration is nullptr");
        }
        // if (!(genericConfig->className() == Lstm::API_CLASS && genericConfig->objectName() == Lstm::API_NAME)) {
        //     return LangCore::Error(LangCore::Error::InvalidArgument, "invalid LstmG2p configuration");
        // }
        return genericConfig.as<Lstm::LstmG2pConfiguration>();
    }

    class LstmG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResult> result;
        LangCore::NO<LangCore::SessionFactory> driver;
        LangCore::NO<LangCore::SessionTask> encoderSession;
        LangCore::NO<LangCore::SessionTask> decodeSession;
        mutable std::shared_mutex mutex;
    };

    LstmG2pTask::LstmG2pTask(const LangCore::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    LstmG2pTask::~LstmG2pTask() = default;

    LangCore::Expected<void> LstmG2pTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "LstmG2p task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Lstm::API_NAME) {
        //     return LangCore::Error(
        //         LangCore::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid LstmG2p task init args name: expected "%1", got "%2")", Lstm::API_NAME,
        //         name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        if (auto res = getObject("driver", "g2pOnnxDriver"); res) {
            impl.driver = res.take().as<LangCore::SessionFactory>();
        } else {
            setState(Failed);
            return res.takeError();
        }

        // Get LstmG2p config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        // Open LstmG2p session (encoder)
        impl.encoderSession = impl.driver->createSession();
        const auto encoderOpenArgs = LangCore::NO<Onnx::SessionOpenArgs>::create();
        encoderOpenArgs->useCpu = false;
        if (auto res = impl.encoderSession->open(config->encoder, encoderOpenArgs); !res) {
            setState(Failed);
            return res;
        }

        impl.decodeSession = impl.driver->createSession();
        const auto predictorOpenArgs = LangCore::NO<Onnx::SessionOpenArgs>::create();
        predictorOpenArgs->useCpu = false;
        if (auto res = impl.decodeSession->open(config->decoder, predictorOpenArgs); !res) {
            setState(Failed);
            return res;
        }

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    LstmG2pTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.driver) {
                setState(Failed);
                return LangCore::Error(LangCore::Error::SessionError, "inference driver not initialized");
            }
        }

        setState(Running);

        // Get configuration
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        if (!input) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::InvalidArgument, "g2p input is nullptr");
        }

        // if (const auto &name = input->objectName(); name != Lstm::API_NAME) {
        //     setState(Failed);
        //     return LangCore::Error(
        //         LangCore::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Lstm::API_NAME, name));
        // }

        const auto g2pInput = input.as<LangCore::G2pStartInput>();

        // Preprocess input word
        if (g2pInput->g2pInput.empty()) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::InvalidArgument, "input words are empty");
        }

        // For now, process only the first word
        const auto &lyric = g2pInput->g2pInput[0];
        auto preprocessedInput = LstmG2pInferenceHelper::preprocessWord(lyric, config);
        if (!preprocessedInput) {
            setState(Failed);
            return preprocessedInput.takeError();
        }

        // Run encoder
        auto encoderInput = LangCore::NO<Onnx::SessionStartInput>::create();
        encoderInput->inputs["input_ids"] = preprocessedInput.take();

        encoderInput->outputs.insert("encoder_outputs");
        encoderInput->outputs.insert("hidden");
        encoderInput->outputs.insert("cell");

        std::unique_lock lock(impl.mutex);
        if (!impl.encoderSession || !impl.encoderSession->isOpen()) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::SessionError, "encoder session is not initialized");
        }

        LangCore::NO<Onnx::SessionResult> encoderResult;
        if (auto encoderExp = impl.encoderSession->start(encoderInput); !encoderExp) {
            setState(Failed);
            return encoderExp.takeError();
        } else {
            auto sessionTaskResult = encoderExp.take();
            if (!sessionTaskResult || sessionTaskResult->objectName() != Onnx::API_NAME) {
                setState(Failed);
                return LangCore::Error(LangCore::Error::SessionError, "invalid encoder result");
            }
            encoderResult = sessionTaskResult.as<Onnx::SessionResult>();
        }

        // Extract encoder outputs
        auto encoderOutputs = LstmG2pInferenceHelper::getTensorFromResult(encoderResult, "encoder_outputs");
        auto hidden = LstmG2pInferenceHelper::getTensorFromResult(encoderResult, "hidden");
        auto cell = LstmG2pInferenceHelper::getTensorFromResult(encoderResult, "cell");

        if (!encoderOutputs || !hidden || !cell) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::SessionError, "failed to get encoder outputs");
        }

        // Run decoder with autoregressive generation
        auto phonemeIds = LstmG2pInferenceHelper::runDecoder(impl.decodeSession, encoderOutputs.take(), hidden.take(),
                                                             cell.take(), config);
        if (!phonemeIds) {
            setState(Failed);
            return phonemeIds.takeError();
        }

        // Decode phonemes
        auto phonemes = LstmG2pInferenceHelper::decodePhonemes(phonemeIds.take(), config);
        if (phonemes->empty()) {
            setState(Failed);
            return phonemes.takeError();
        }

        auto phonemes_ = phonemes.take();

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResult>::create(LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS,
                                                                   LangCore::G2P_API_LEVEL);
        g2pResult->g2pResult = {
            LangCore::G2pRes(lyric, "eng",
                             std::accumulate(phonemes_.begin() + 1, phonemes_.end(), phonemes_[0],
                                             [](const std::string &a, const std::string &b) { return a + " " + b; }),
                             {}, "copy", true)};

        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangCore::Expected<void> LstmG2pTask::startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                                     const StartAsyncCallback &callback) {
        // TODO:
        return LangCore::Error(LangCore::Error::NotImplemented);
    }

    bool LstmG2pTask::stop() {
        __stdc_impl_t;
        bool flag = true;
        for (auto &session : {impl.encoderSession, impl.decodeSession}) {
            if (session) {
                flag &= session->stop();
            }
        }
        setState(Terminated);
        return flag;
    }

    LangCore::NO<LangCore::TaskResult> LstmG2pTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }

    LangCore::Expected<LangCore::NO<ITensor>>
    LstmG2pInferenceHelper::preprocessWord(const std::string &word,
                                           const LangCore::NO<Lstm::LstmG2pConfiguration> &config) {
        const std::string processedWord = stdc::to_lower(word);
        stdc::trim(processedWord);

        std::vector<int64_t> indices;
        indices.push_back(config->bosIdx); // BOS

        for (const char c : processedWord) {
            std::string charStr(1, c);
            if (auto it = config->charVocab.find(charStr); it != config->charVocab.end()) {
                indices.push_back(it->second);
            } else {
                indices.push_back(config->unkIdx);
            }
        }

        indices.push_back(config->eosIdx); // EOS

        const std::vector shape{static_cast<int64_t>(indices.size())};
        if (auto exp = Tensor::createFromView<int64_t>(shape, stdc::array_view<int64_t>{indices}); exp) {
            return exp.take();
        }
        return LangCore::Error(LangCore::Error::InvalidArgument,
                               stdc::formatN("Failed to create tensor for word: %1", word));
    }

    LangCore::Expected<LangCore::NO<ITensor>>
    LstmG2pInferenceHelper::getTensorFromResult(const LangCore::NO<Onnx::SessionResult> &result,
                                                const std::string &name) {

        const auto it = result->outputs.find(name);
        if (it == result->outputs.end()) {
            return LangCore::Error(LangCore::Error::SessionError,
                                   stdc::formatN("output '%1' not found in session result", name));
        }
        return it->second;
    }

    LangCore::Expected<std::vector<int64_t>>
    LstmG2pInferenceHelper::runDecoder(const LangCore::NO<LangCore::SessionTask> &decodeSession,
                                       const LangCore::NO<ITensor> &encoderOutputs, const LangCore::NO<ITensor> &hidden,
                                       const LangCore::NO<ITensor> &cell,
                                       const LangCore::NO<Lstm::LstmG2pConfiguration> &config) {

        std::vector<int64_t> phonemeIds;
        const int64_t maxLen = config->maxLen > 0 ? config->maxLen : 48;

        // Initialize decoder input with BOS - 创建1D张量
        std::vector<int64_t> decoderInitData{config->bosIdx};
        std::vector<int64_t> decoderInitShape{1};
        LangCore::NO<ITensor> decoderInput;

        if (auto exp = Tensor::createFromView<int64_t>(decoderInitShape, stdc::array_view<int64_t>{decoderInitData});
            exp) {
            decoderInput = exp.take();
        } else {
            return exp.takeError();
        }

        auto currentHidden = hidden;
        auto currentCell = cell;

        for (int64_t i = 0; i < maxLen; ++i) {
            auto decoderSessionInput = LangCore::NO<Onnx::SessionStartInput>::create();
            decoderSessionInput->inputs["decoder_input"] = decoderInput;
            decoderSessionInput->inputs["hidden"] = currentHidden;
            decoderSessionInput->inputs["cell"] = currentCell;
            decoderSessionInput->inputs["encoder_outputs"] = encoderOutputs;

            decoderSessionInput->outputs.insert("output");
            decoderSessionInput->outputs.insert("hidden_new");
            decoderSessionInput->outputs.insert("cell_new");
            decoderSessionInput->outputs.insert("attention_weights");

            LangCore::NO<Onnx::SessionResult> decoderResult;
            if (auto decoderExp = decodeSession->start(decoderSessionInput); !decoderExp) {
                return decoderExp.takeError();
            } else {
                auto sessionTaskResult = decoderExp.take();
                if (!sessionTaskResult || sessionTaskResult->objectName() != Onnx::API_NAME) {
                    return LangCore::Error(LangCore::Error::SessionError, "invalid decoder result");
                }
                decoderResult = sessionTaskResult.as<Onnx::SessionResult>();
            }

            auto output = getTensorFromResult(decoderResult, "output");
            currentHidden = getTensorFromResult(decoderResult, "hidden_new").take();
            currentCell = getTensorFromResult(decoderResult, "cell_new").take();

            if (!output || !currentHidden || !currentCell) {
                return LangCore::Error(LangCore::Error::SessionError, "failed to get decoder outputs");
            }

            // Get predicted phoneme ID (argmax)
            const auto outputTensor = output.take();
            if (outputTensor->dataType() != ITensor::Float) {
                return LangCore::Error(LangCore::Error::SessionError, "decoder output is not float");
            }

            auto outputView = outputTensor->view<float>();
            if (outputView.empty()) {
                return LangCore::Error(LangCore::Error::SessionError, "decoder output is empty");
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
            if (predictedId == config->eosIdx) {
                break;
            }

            phonemeIds.push_back(predictedId);

            // Update decoder input for next step - 创建1D张量
            std::vector nextInputData{predictedId};
            std::vector<int64_t> nextInputShape{1};
            if (auto exp = Tensor::createFromView<int64_t>(nextInputShape, stdc::array_view<int64_t>{nextInputData});
                exp) {
                decoderInput = exp.take();
            } else {
                return exp.takeError();
            }
        }

        return phonemeIds;
    }

    LangCore::Expected<std::vector<std::string>>
    LstmG2pInferenceHelper::decodePhonemes(const std::vector<int64_t> &phonemeIds,
                                           const LangCore::NO<Lstm::LstmG2pConfiguration> &config) {
        std::vector<std::string> phonemes;

        std::unordered_map<int64_t, std::string> idToPhoneme;
        for (const auto &[phoneme, id] : config->phonemeVocab) {
            idToPhoneme[id] = phoneme;
        }

        for (const int64_t id : phonemeIds) {
            // Skip special tokens
            if (id == config->bosIdx || id == config->eosIdx || id == config->padIdx || id == config->unkIdx) {
                continue;
            }

            if (auto it = idToPhoneme.find(id); it != idToPhoneme.end()) {
                phonemes.push_back(it->second);
            }
        }

        return phonemes;
    }
} // namespace LangPlugins
