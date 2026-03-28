#ifndef LANGPLUGINS_LSTMG2PTASK_H
#define LANGPLUGINS_LSTMG2PTASK_H

#include <LangCore/Support/Tensor.h>
#include <LangCore/Task/Task.h>

#include <LangCore/Task/SessionTask.h>

namespace LangPlugins::LstmG2p::V1
{
    class LstmG2pTask : public LangCore::Task {
    public:
        explicit LstmG2pTask(const LangCore::ModuleSpec *spec);
        ~LstmG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    class LstmG2pInferenceHelper {
    public:
        // Preprocess word into tensor
        static LangCore::Expected<LangCore::NO<LangCore::ITensor>> preprocessWord(const std::string &word,
                                                                                  std::map<std::string, int> charVocab,
                                                                                  int bosIdx, int eosIdx, int unkIdx);

        // Get tensor from session result by name
        static LangCore::Expected<LangCore::NO<LangCore::ITensor>>
        getTensorFromResult(const LangCore::NO<LangCore::SessionResult> &result, const std::string &name);

        // Run decoder with autoregressive generation
        static LangCore::Expected<std::vector<int64_t>>
        runDecoder(const LangCore::NO<LangCore::SessionTask> &decodeSession,
                   const LangCore::NO<LangCore::ITensor> &encoderOutputs, const LangCore::NO<LangCore::ITensor> &hidden,
                   const LangCore::NO<LangCore::ITensor> &cell, int maxLen, int bosIdx, int eosIdx);

        // Decode phoneme indices to phoneme strings
        static LangCore::Expected<std::vector<std::string>>
        decodePhonemes(const std::vector<int64_t> &phonemeIds, const std::map<std::string, int> &phonemeVocab,
                       int bosIdx, int eosIdx, int padIdx, int unkIdx);
    };
} // namespace LangPlugins::LstmG2p::V1

#endif // LANGPLUGINS_LSTMG2PTASK_H
