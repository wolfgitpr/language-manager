#ifndef G2PMODEL_H
#define G2PMODEL_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct OrtApi;
struct OrtEnv;
struct OrtSession;
struct OrtSessionOptions;
struct OrtRunOptions;
struct OrtMemoryInfo;
struct OrtAllocator;
struct OrtValue;

namespace LangMgr
{
    enum class ExecutionProvider { CPU, DML, CUDA };

    class G2pDriver;

    class G2pModel {
    public:
        explicit G2pModel(G2pDriver *driver, const std::filesystem::path &modelPath, ExecutionProvider provider,
                          int device_id);
        ~G2pModel();

        bool is_open() const;

        std::vector<std::string> forward(const std::string &word);
        bool forward(const std::vector<int64_t> &input_ids, std::vector<int64_t> &phoneme_ids) const;

        void terminate() const;

        void loadVocab(const std::string &vocab_path);
        void loadConfig(const std::string &config_path);

    private:
        G2pDriver *m_driver;

        OrtEnv *m_env = nullptr;
        OrtSession *m_session = nullptr;
        OrtSessionOptions *m_session_options = nullptr;
        OrtRunOptions *m_run_options = nullptr;
        OrtMemoryInfo *m_memory_info = nullptr;

        std::unordered_map<std::string, int64_t> char_vocab;
        std::unordered_map<std::string, int64_t> phoneme_vocab;
        std::unordered_map<int64_t, std::string> idx_to_phoneme;

        int64_t UNK_IDX;
        int64_t PAD_IDX;
        int64_t BOS_IDX;
        int64_t EOS_IDX;
        int64_t max_len = 48;

        std::vector<int64_t> preprocess_word(const std::string &word);
        std::vector<std::string> decode_phonemes(const std::vector<int64_t> &indices);

        static std::string to_lower(const std::string &str);
        static std::string trim(const std::string &str);

        const OrtApi *api() const;
    };

} // namespace LangMgr

#endif // G2PMODEL_H
