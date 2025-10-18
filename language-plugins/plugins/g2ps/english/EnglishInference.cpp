#include "EnglishInference.h"

#include <fstream>
#include <mutex>
#include <numeric>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangPlugins/Core/Tensor.h>
#include <LangPlugins/Inference/InferenceDriver.h>
#include <LangPlugins/Inference/InferenceSession.h>

#include <inferutil/Driver.h>
#include <inferutil/TensorHelper.h>

namespace LangPlugins
{
    static LangMgr::Expected<LangMgr::NO<Lstm::LstmG2pConfiguration>> getConfig(const LangMgr::InferenceSpec *spec) {

        const auto genericConfig = spec->configuration();
        if (!genericConfig) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "LstmG2p configuration is nullptr");
        }
        if (!(genericConfig->className() == Lstm::API_CLASS && genericConfig->objectName() == Lstm::API_NAME)) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid LstmG2p configuration");
        }
        return genericConfig.as<Lstm::LstmG2pConfiguration>();
    }

    class EnglishInference::Impl {
    public:
        LangMgr::NO<Lstm::LstmG2pResult> result;
        LangMgr::NO<InferenceDriver> driver;
        LangMgr::NO<InferenceSession> encoderSession;
        LangMgr::NO<InferenceSession> decodeSession;
        mutable std::shared_mutex mutex;
    };

    EnglishInference::EnglishInference(const LangMgr::InferenceSpec *spec) :
        Inference(spec), _impl(std::make_unique<Impl>()) {}

    EnglishInference::~EnglishInference() = default;

    LangMgr::Expected<void> EnglishInference::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "LstmG2p task init args is nullptr");
        }
        if (auto name = args->objectName(); name != Lstm::API_NAME) {
            return LangMgr::Error(
                LangMgr::Error::InvalidArgument,
                stdc::formatN(R"(invalid LstmG2p task init args name: expected "%1", got "%2")", Lstm::API_NAME, name));
        }
        auto LstmG2pArgs = args.as<Lstm::LstmG2pInitArgs>();

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        if (auto res = inferutil::getInferenceDriver(this); res) {
            impl.driver = res.take();
        } else {
            setState(Failed);
            return res.takeError();
        }

        // Get LstmG2p config
        auto expConfig = getConfig(spec());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        // Open LstmG2p session (encoder)
        impl.encoderSession = impl.driver->createSession();
        const auto encoderOpenArgs = LangMgr::NO<Onnx::SessionOpenArgs>::create();
        encoderOpenArgs->useCpu = false;
        if (auto res = impl.encoderSession->open(config->encoder, encoderOpenArgs); !res) {
            setState(Failed);
            return res;
        }

        impl.decodeSession = impl.driver->createSession();
        const auto predictorOpenArgs = LangMgr::NO<Onnx::SessionOpenArgs>::create();
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

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
    EnglishInference::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.driver) {
                setState(Failed);
                return LangMgr::Error(LangMgr::Error::SessionError, "inference driver not initialized");
            }
        }

        setState(Running);

        // Get configuration
        auto expConfig = getConfig(spec());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        if (!input) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "g2p input is nullptr");
        }

        if (const auto &name = input->objectName(); name != Lstm::API_NAME) {
            setState(Failed);
            return LangMgr::Error(
                LangMgr::Error::InvalidArgument,
                stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Lstm::API_NAME, name));
        }

        const auto g2pInput = input.as<Lstm::LstmG2pStartInput>();

        // Preprocess input word
        if (g2pInput->words.empty()) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "input words are empty");
        }

        // For now, process only the first word
        const auto &word = g2pInput->words[0];
        auto preprocessedInput = EnglishInferenceHelper::preprocessWord(word.text, config);
        if (!preprocessedInput) {
            setState(Failed);
            return preprocessedInput.takeError();
        }

        // Run encoder
        auto encoderInput = LangMgr::NO<Onnx::SessionStartInput>::create();
        encoderInput->inputs["input_ids"] = preprocessedInput.take();

        std::unique_lock lock(impl.mutex);
        if (!impl.encoderSession || !impl.encoderSession->isOpen()) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::SessionError, "encoder session is not initialized");
        }

        LangMgr::NO<Onnx::SessionResult> encoderResult;
        if (auto encoderExp = impl.encoderSession->start(encoderInput); !encoderExp) {
            setState(Failed);
            return encoderExp.takeError();
        } else {
            auto sessionTaskResult = encoderExp.take();
            if (!sessionTaskResult || sessionTaskResult->objectName() != Onnx::API_NAME) {
                setState(Failed);
                return LangMgr::Error(LangMgr::Error::SessionError, "invalid encoder result");
            }
            encoderResult = sessionTaskResult.as<Onnx::SessionResult>();
        }

        // Extract encoder outputs
        auto encoderOutputs = EnglishInferenceHelper::getTensorFromResult(encoderResult, "encoder_outputs");
        auto hidden = EnglishInferenceHelper::getTensorFromResult(encoderResult, "hidden");
        auto cell = EnglishInferenceHelper::getTensorFromResult(encoderResult, "cell");

        if (!encoderOutputs || !hidden || !cell) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::SessionError, "failed to get encoder outputs");
        }

        // Run decoder with autoregressive generation
        auto phonemeIds = EnglishInferenceHelper::runDecoder(impl.decodeSession, encoderOutputs.take(), hidden.take(),
                                                             cell.take(), config);
        if (!phonemeIds) {
            setState(Failed);
            return phonemeIds.takeError();
        }

        // Decode phonemes
        auto phonemes = EnglishInferenceHelper::decodePhonemes(phonemeIds.take(), config);
        if (!phonemes) {
            setState(Failed);
            return phonemes.takeError();
        }

        // Create result
        auto g2pResult = LangMgr::NO<Lstm::LstmG2pResult>::create();
        g2pResult->phonemes = phonemes.take();

        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangMgr::Expected<void> EnglishInference::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                         const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool EnglishInference::stop() {
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

    LangMgr::NO<LangMgr::TaskResult> EnglishInference::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }

    LangMgr::Expected<LangMgr::NO<ITensor>>
    EnglishInferenceHelper::preprocessWord(const std::string &word,
                                           const LangMgr::NO<Lstm::LstmG2pConfiguration> &config) {

        // Convert word to lowercase and preprocess
        const std::string processedWord = stdc::to_lower(word);
        stdc::trim(processedWord);

        // Convert characters to indices
        std::vector<int64_t> indices;
        indices.push_back(config->bosIdx); // BOS

        for (const char c : processedWord) {
            if (auto it = config->charVocab.find(std::string(1, c)); it != config->charVocab.end()) {
                indices.push_back(it->second);
            } else {
                indices.push_back(config->unkIdx);
            }
        }

        indices.push_back(config->eosIdx); // EOS

        return createTensor(indices, {1, static_cast<int64_t>(indices.size())});
    }

    LangMgr::Expected<LangMgr::NO<ITensor>>
    EnglishInferenceHelper::getTensorFromResult(const LangMgr::NO<Onnx::SessionResult> &result,
                                                const std::string &name) {

        const auto it = result->outputs.find(name);
        if (it == result->outputs.end()) {
            return LangMgr::Error(LangMgr::Error::SessionError,
                                  stdc::formatN("output '%1' not found in session result", name));
        }
        return it->second;
    }

    LangMgr::Expected<std::vector<int64_t>>
    EnglishInferenceHelper::runDecoder(const LangMgr::NO<InferenceSession> &decodeSession,
                                       const LangMgr::NO<ITensor> &encoderOutputs, const LangMgr::NO<ITensor> &hidden,
                                       const LangMgr::NO<ITensor> &cell,
                                       const LangMgr::NO<Lstm::LstmG2pConfiguration> &config) {

        std::vector<int64_t> phonemeIds;
        const int64_t maxLen = config->maxLen > 0 ? config->maxLen : 48;

        // Initialize decoder input with BOS
        auto decoderInputExp = createTensor(std::vector<int64_t>{config->bosIdx}, {1, 1});
        if (!decoderInputExp) {
            return decoderInputExp.takeError();
        }
        auto decoderInput = decoderInputExp.take();

        auto currentHidden = hidden;
        auto currentCell = cell;

        for (int64_t i = 0; i < maxLen; ++i) {
            auto decoderSessionInput = LangMgr::NO<Onnx::SessionStartInput>::create();
            decoderSessionInput->inputs["decoder_input"] = decoderInput;
            decoderSessionInput->inputs["hidden"] = currentHidden;
            decoderSessionInput->inputs["cell"] = currentCell;
            decoderSessionInput->inputs["encoder_outputs"] = encoderOutputs;

            LangMgr::NO<Onnx::SessionResult> decoderResult;
            if (auto decoderExp = decodeSession->start(decoderSessionInput); !decoderExp) {
                return decoderExp.takeError();
            } else {
                auto sessionTaskResult = decoderExp.take();
                if (!sessionTaskResult || sessionTaskResult->objectName() != Onnx::API_NAME) {
                    return LangMgr::Error(LangMgr::Error::SessionError, "invalid decoder result");
                }
                decoderResult = sessionTaskResult.as<Onnx::SessionResult>();
            }

            auto output = getTensorFromResult(decoderResult, "output");
            currentHidden = getTensorFromResult(decoderResult, "hidden").take();
            currentCell = getTensorFromResult(decoderResult, "cell").take();

            if (!output || !currentHidden || !currentCell) {
                return LangMgr::Error(LangMgr::Error::SessionError, "failed to get decoder outputs");
            }

            // Get predicted phoneme ID (argmax)
            const auto outputTensor = output.take();
            if (outputTensor->dataType() != ITensor::Float) {
                return LangMgr::Error(LangMgr::Error::SessionError, "decoder output is not float");
            }

            auto outputView = outputTensor->view<float>();
            if (outputView.empty()) {
                return LangMgr::Error(LangMgr::Error::SessionError, "decoder output is empty");
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

            // Update decoder input for next step
            auto nextInputExp = createTensor(std::vector{predictedId}, {1, 1});
            if (!nextInputExp) {
                return nextInputExp.takeError();
            }
            decoderInput = nextInputExp.take();
        }

        return phonemeIds;
    }

    LangMgr::Expected<std::vector<std::string>>
    EnglishInferenceHelper::decodePhonemes(const std::vector<int64_t> &phonemeIds,
                                           const LangMgr::NO<Lstm::LstmG2pConfiguration> &config) {
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

    template <typename T>
    LangMgr::Expected<LangMgr::NO<ITensor>> EnglishInferenceHelper::createTensor(const std::vector<T> &data,
                                                                                 const std::vector<int64_t> &shape) {

        int64_t totalElements = 1;
        for (const auto &dim : shape) {
            if (dim <= 0) {
                return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid tensor shape dimension");
            }
            totalElements *= dim;
        }

        if (data.size() != static_cast<size_t>(totalElements)) {
            return LangMgr::Error(
                LangMgr::Error::InvalidArgument,
                stdc::formatN("data size (%1) does not match tensor shape (%2)", data.size(), totalElements));
        }

        auto helperExp = inferutil::TensorHelper<T>::createFor1DArray(data.size());
        if (!helperExp) {
            return helperExp.takeError();
        }

        auto helper = helperExp.take();

        for (const auto &value : data) {
            if (!helper.write(value)) {
                return LangMgr::Error(LangMgr::Error::SessionError, "failed to write data to tensor");
            }
        }

        if (!helper.isComplete()) {
            return LangMgr::Error(LangMgr::Error::SessionError, "tensor data writing incomplete");
        }

        return helper.take().template as<ITensor>();
    }

    template LangMgr::Expected<LangMgr::NO<ITensor>>
    EnglishInferenceHelper::createTensor<int64_t>(const std::vector<int64_t> &, const std::vector<int64_t> &);

    template LangMgr::Expected<LangMgr::NO<ITensor>>
    EnglishInferenceHelper::createTensor<float>(const std::vector<float> &, const std::vector<int64_t> &);
} // namespace LangPlugins
