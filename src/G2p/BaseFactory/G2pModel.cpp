#include "G2pModel.h"
#include "G2pDriver.h"

#include <stdcorelib/str.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <iostream>

#include <onnxruntime_cxx_api.h>

namespace LangMgr
{
    G2pModel::G2pModel(G2pDriver *driver, const std::filesystem::path &modelPath, const ExecutionProvider provider,
                       int device_id) : m_driver(driver) {

        if (!m_driver || !m_driver->isLoaded()) {
            std::cout << "G2pDriver not loaded" << std::endl;
            return;
        }

        const auto *ortApi = api();

        ortApi->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "G2pModel", &m_env);
        ortApi->CreateSessionOptions(&m_session_options);
        ortApi->SetInterOpNumThreads(m_session_options, 4);
        ortApi->CreateRunOptions(&m_run_options);

#ifdef _WIN_X86
        ortApi->CreateCpuMemoryInfo(OrtDeviceAllocator, OrtMemTypeCPU, &m_memory_info);
#else
        ortApi->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &m_memory_info);
#endif

        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
        case ExecutionProvider::DML:
            {
                std::string errorMessage;
                if (!initDirectML(ortApi, m_session_options, device_id, &errorMessage)) {
                    std::cout << "Failed to enable Dml: " << errorMessage << ". Falling back to CPU." << std::endl;
                } else {
                    std::cout << "Use Dml execution provider" << std::endl;
                }
                break;
            }
#endif

#if ONNXRUNTIME_ENABLE_CUDA
        case ExecutionProvider::CUDA:
            {
                std::string errorMessage;
                if (!initCUDA(ortApi, m_session_options, device_id, &errorMessage)) {
                    std::cout << "Failed to enable CUDA: " << errorMessage << std::endl;
                } else {
                    std::cout << "Using CUDA execution provider" << std::endl;
                }
                break;
            }
#endif

        default:
            break;
        }

        try {
#ifdef _WIN32
            ortApi->CreateSession(m_env, modelPath.wstring().c_str(), m_session_options, &m_session);
#else
            ortApi->CreateSession(m_env, modelPath.c_str(), m_session_options, &m_session);
#endif
        }
        catch (const std::exception &e) {
            std::cout << "Failed to create session: " << e.what() << std::endl;
        }

        const auto vocab_path = modelPath.parent_path() / "vocab.yaml";
        if (!std::filesystem::exists(vocab_path)) {
            std::cout << "Vocab file not found at: " << vocab_path << std::endl;
            return;
        }
        loadVocab(vocab_path.string());

        const auto config_path = modelPath.parent_path() / "config.yaml";
        if (!std::filesystem::exists(config_path)) {
            std::cout << "Config file not found at: " << config_path << std::endl;
            return;
        }
        loadConfig(config_path.string());
    }

    G2pModel::~G2pModel() {
        if (const auto *ortApi = api()) {
            if (m_session)
                ortApi->ReleaseSession(m_session);
            if (m_session_options)
                ortApi->ReleaseSessionOptions(m_session_options);
            if (m_run_options)
                ortApi->ReleaseRunOptions(m_run_options);
            if (m_memory_info)
                ortApi->ReleaseMemoryInfo(m_memory_info);
            if (m_env)
                ortApi->ReleaseEnv(m_env);
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
            if (auto it = char_vocab.find(char_str); it != char_vocab.end()) {
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

            if (auto it = idx_to_phoneme.find(idx); it != idx_to_phoneme.end()) {
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

    const OrtApi *G2pModel::api() const { return m_driver ? m_driver->api() : nullptr; }

    void G2pModel::terminate() const {
        if (const auto *ortApi = api(); ortApi && m_run_options) {
            ortApi->RunOptionsSetTerminate(m_run_options);
        }
    }

    bool G2pModel::is_open() const { return m_session != nullptr; }

    std::vector<std::string> G2pModel::forward(const std::string &word) {
        std::vector<int64_t> phoneme_ids;
        if (!forward(preprocess_word(word), phoneme_ids)) {
            std::cerr << "G2p forward error: " << std::endl;
        }
        return decode_phonemes(phoneme_ids);
    }

    bool G2pModel::forward(const std::vector<int64_t> &input_ids, std::vector<int64_t> &phoneme_ids) const {
        if (!m_session) {
            std::cerr << "G2p session is not initialized." << std::endl;
            return false;
        }

        const auto *ortApi = api();
        if (!ortApi) {
            std::cerr << "ORT API not available." << std::endl;
            return false;
        }

        try {
            const std::vector input_shape = {static_cast<int64_t>(input_ids.size())};

            OrtValue *input_tensor = nullptr;
            ortApi->CreateTensorWithDataAsOrtValue(
                m_memory_info, const_cast<int64_t *>(input_ids.data()), input_ids.size() * sizeof(int64_t),
                input_shape.data(), input_shape.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &input_tensor);

            const char *input_names[] = {"input_ids"};
            const char *output_names[] = {"phoneme_ids"};

            OrtValue *output_tensor = nullptr;

            ortApi->Run(m_session, m_run_options, input_names, &input_tensor, 1, output_names, 1, &output_tensor);
            if (!output_tensor) {
                ortApi->ReleaseValue(input_tensor);
                std::cerr << "Invalid output from ONNX model" << std::endl;
                return false;
            }

            OrtTensorTypeAndShapeInfo *output_info = nullptr;
            ortApi->GetTensorTypeAndShape(output_tensor, &output_info);

            // Get the number of dimensions (rank)
            size_t num_dims = 0;
            ortApi->GetDimensionsCount(output_info, &num_dims);

            std::vector<int64_t> output_shape(num_dims);
            ortApi->GetDimensions(output_info, output_shape.data(), num_dims);

            // Calculate the total number of elements
            size_t total_elements = 1;
            for (size_t i = 0; i < num_dims; ++i) {
                total_elements *= output_shape[i];
            }

            int64_t *output_data = nullptr;
            ortApi->GetTensorMutableData(output_tensor, reinterpret_cast<void **>(&output_data));

            // Use total_elements for assign
            phoneme_ids.assign(output_data, output_data + total_elements);

            ortApi->ReleaseTensorTypeAndShapeInfo(output_info);
            ortApi->ReleaseValue(output_tensor);
            ortApi->ReleaseValue(input_tensor);

            return true;
        }
        catch (const std::exception &e) {
            std::cerr << "ONNX inference failed: " << e.what() << std::endl;
            return false;
        }
    }
} // namespace LangMgr
