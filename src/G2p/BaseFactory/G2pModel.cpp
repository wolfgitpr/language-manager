#include "G2pModel.h"


#include <stdcorelib/str.h>
#include <yaml-cpp/yaml.h>

#include <dsinfer/Api/Drivers/Onnx/OnnxDriverApi.h>
#include <dsinfer/Inference/InferenceDriver.h>
#include <synthrt/Core/Contribute.h>
#include <synthrt/Core/SynthUnit.h>
namespace LangMgr
{
    static srt::Expected<srt::NO<ds::InferenceDriver>> getInferenceDriver(const srt::SynthUnit *su) {
        if (!su) {
            return srt::Error(srt::Error::SessionError, "SynthUnit is nullptr");
        }
        const auto inferenceCate = su->category("inference");
        const auto dsdriverObject = inferenceCate->getFirstObject("dsdriver");

        if (!dsdriverObject) {
            return srt::Error(srt::Error::SessionError, "could not find dsdriver");
        }

        auto onnxDriver = dsdriverObject.as<ds::InferenceDriver>();

        return onnxDriver;
    }

    G2pModel::G2pModel(const srt::SynthUnit *su) : m_su(su) {}

    G2pModel::~G2pModel() = default;

    srt::Expected<void> G2pModel::open(const std::filesystem::path &modelPath) {
        loadVocab((modelPath / "vocab.yaml").string());
        loadConfig((modelPath / "config.yaml").string());

        if (!m_driver) {
            if (auto exp = getInferenceDriver(m_su); !exp) {
                return exp.takeError();
            } else {
                m_driver = exp.take();
            }
        }
        auto session = m_driver->createSession();
        if (!session) {
            return srt::Error(srt::Error::SessionError, "could not create session");
        }
        if (auto exp = session->open(modelPath / "model.onnx", srt::NO<ds::Api::Onnx::SessionOpenArgs>::create());
            !exp) {
            return exp.takeError();
        }
        m_session = std::move(session);
        return srt::Expected<void>();
    }

    void G2pModel::close() {
        if (m_session) {
            m_session->stop();
            m_session->close();
            m_session.reset();
        }
    }

    void G2pModel::terminate() const {
        if (m_session) {
            m_session->stop();
        }
    }

    void G2pModel::loadVocab(const std::string &vocab_path) {
        YAML::Node vocab_data = YAML::LoadFile(vocab_path);

        YAML::Node char_vocab_node = vocab_data["char_vocab"];
        for (YAML::const_iterator it = char_vocab_node.begin(); it != char_vocab_node.end(); ++it) {
            char_vocab[it->first.as<std::string>()] = it->second.as<int64_t>();
        }

        YAML::Node phoneme_vocab_node = vocab_data["phoneme_vocab"];
        for (YAML::const_iterator it = phoneme_vocab_node.begin(); it != phoneme_vocab_node.end(); ++it) {
            phoneme_vocab[it->first.as<std::string>()] = it->second.as<int64_t>();
        }

        YAML::Node idx_to_phoneme_node = vocab_data["idx_to_phoneme"];
        for (YAML::const_iterator it = idx_to_phoneme_node.begin(); it != idx_to_phoneme_node.end(); ++it) {
            idx_to_phoneme[it->first.as<std::int64_t>()] = it->second.as<std::string>();
        }
    }

    void G2pModel::loadConfig(const std::string &config_path) {
        YAML::Node config = YAML::LoadFile(config_path);

        UNK_IDX = config["unk_idx"].as<int64_t>();
        PAD_IDX = config["pad_idx"].as<int64_t>();
        BOS_IDX = config["bos_idx"].as<int64_t>();
        EOS_IDX = config["eos_idx"].as<int64_t>();

        if (config["max_len"]) {
            max_len = config["max_len"].as<int64_t>();
        }
    }

    std::vector<int64_t> G2pModel::preprocess_word(const std::string &word) {
        const std::string processed_word = to_lower(trim(word));
        std::vector<int64_t> word_indices;

        word_indices.push_back(BOS_IDX);

        for (const char c : processed_word) {
            std::string char_str(1, c);
            auto it = char_vocab.find(char_str);
            if (it != char_vocab.end()) {
                word_indices.push_back(it->second);
            } else {
                word_indices.push_back(UNK_IDX);
            }
        }

        word_indices.push_back(EOS_IDX);

        return word_indices;
    }

    std::vector<std::string> G2pModel::decode_phonemes(const std::vector<int64_t> &indices) {
        std::vector<std::string> phonemes;

        for (int64_t idx : indices) {
            if (idx == BOS_IDX || idx == EOS_IDX || idx == PAD_IDX || idx == UNK_IDX) {
                continue;
            }

            auto it = idx_to_phoneme.find(idx);
            if (it != idx_to_phoneme.end()) {
                phonemes.push_back(it->second);
            }
        }

        return phonemes;
    }

    std::string G2pModel::to_lower(const std::string &str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), tolower);
        return result;
    }

    std::string G2pModel::trim(const std::string &str) {
        const size_t start = str.find_first_not_of(" \t\n\r");
        const size_t end = str.find_last_not_of(" \t\n\r");

        if (start == std::string::npos || end == std::string::npos) {
            return "";
        }

        return str.substr(start, end - start + 1);
    }

    bool G2pModel::is_open() const { return m_session != nullptr; }

    srt::Expected<std::vector<std::string>> G2pModel::forward(const std::string &word) {
        std::vector<int64_t> phoneme_ids;
        if (srt::Expected<void> result = forward(preprocess_word(word), phoneme_ids); !result) {
            std::cerr << "G2p forward error: " << result.error().message() << std::endl;
        }
        return decode_phonemes(phoneme_ids);
    }

    template <typename T>
    static srt::Expected<std::vector<T>> extractTensor(const std::map<std::string, srt::NO<ds::ITensor>> &outputs,
                                                       const std::string &name) {

        const auto it = outputs.find(name);
        if (it == outputs.end()) {
            return srt::Error(srt::Error::SessionError, "missing output: " + name);
        }
        const auto &tensor = it->second;
        if (tensor->dataType() != ds::tensor_traits<T>::data_type) {
            return srt::Error(srt::Error::SessionError, "data type mismatch: " + name);
        }
        const auto data = tensor->view<T>();
        if (data.empty()) {
            return srt::Error(srt::Error::SessionError, "could not get output data: " + name);
        }
        return data.vec();
    }

    template <>
    srt::Expected<std::vector<bool>> extractTensor(const std::map<std::string, srt::NO<ds::ITensor>> &outputs,
                                                   const std::string &name) {

        const auto it = outputs.find(name);
        if (it == outputs.end()) {
            return srt::Error(srt::Error::SessionError, "missing output: " + name);
        }
        const auto &tensor = it->second;
        if (tensor->dataType() != ds::tensor_traits<bool>::data_type) {
            return srt::Error(srt::Error::SessionError, "data type mismatch: " + name);
        }
        const auto data = tensor->rawView();
        if (data.empty()) {
            return srt::Error(srt::Error::SessionError, "could not get output data: " + name);
        }
        std::vector output(data.size(), false);
        for (size_t i = 0; i < data.size(); ++i) {
            output[i] = data[i] != std::byte{0};
        }
        return output;
    }

    // Forward pass through the model: takes waveform and threshold as inputs, returns f0 and uv as outputs
    srt::Expected<void> G2pModel::forward(const std::vector<int64_t> &input_ids,
                                          std::vector<int64_t> &phoneme_ids) const {
        if (!m_session) {
            return srt::Error(srt::Error::SessionError, "G2p session is not initialized.");
        }
        const size_t n_samples = input_ids.size();
        const std::vector input_shape = {static_cast<int64_t>(n_samples)};

        const auto sessionInput = srt::NO<ds::Api::Onnx::SessionStartInput>::create();

        if (auto exp = ds::Tensor::createFromView<int64_t>(input_shape, input_ids); !exp) {
            return exp.takeError();
        } else {
            sessionInput->inputs["input_ids"] = exp.take();
        }
        sessionInput->outputs = {"phoneme_ids"};

        if (auto exp = m_session->start(sessionInput); !exp) {
            return exp.takeError();
        }
        const auto result = m_session->result().as<ds::Api::Onnx::SessionResult>();
        if (!result) {
            return srt::Error(srt::Error::SessionError, "could not get G2p session result");
        }

        if (auto exp = extractTensor<int64_t>(result->outputs, "phoneme_ids"); exp) {
            phoneme_ids = exp.take();
        } else {
            return exp.takeError();
        }

        return srt::Expected<void>();
    }
} // namespace LangMgr
