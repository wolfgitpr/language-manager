#ifndef G2PMODEL_H
#define G2PMODEL_H

#include <filesystem>
#include <string>
#include <vector>

#include <dsinfer/Inference/InferenceDriver.h>
#include <dsinfer/Inference/InferenceSession.h>
#include <synthrt/Core/NamedObject.h>
#include <synthrt/Support/Expected.h>

namespace srt
{
    class SynthUnit;
}

namespace LangMgr
{
    class G2pModel {
    public:
        explicit G2pModel(const srt::SynthUnit *su);
        ~G2pModel();

        srt::Expected<void> open(const std::filesystem::path &modelPath);
        void close();
        bool is_open() const;

        srt::Expected<std::vector<std::string>> forward(const std::string &word);
        srt::Expected<void> forward(const std::vector<int64_t> &input_ids, std::vector<int64_t> &phoneme_ids) const;

        void terminate() const;

    private:
        const srt::SynthUnit *const m_su = nullptr;
        srt::NO<ds::InferenceDriver> m_driver;
        srt::NO<ds::InferenceSession> m_session;

        std::unordered_map<std::string, int64_t> char_vocab;
        std::unordered_map<std::string, int64_t> phoneme_vocab;
        std::unordered_map<int64_t, std::string> idx_to_phoneme;

        int64_t UNK_IDX;
        int64_t PAD_IDX;
        int64_t BOS_IDX;
        int64_t EOS_IDX;
        int64_t max_len = 48;

        void loadVocab(const std::string &vocab_path);

        void loadConfig(const std::string &config_path);

        std::vector<int64_t> preprocess_word(const std::string &word);

        std::vector<std::string> decode_phonemes(const std::vector<int64_t> &indices);

        static std::string to_lower(const std::string &str);

        static std::string trim(const std::string &str);
    };

} // namespace LangMgr

#endif // G2PMODEL_H
