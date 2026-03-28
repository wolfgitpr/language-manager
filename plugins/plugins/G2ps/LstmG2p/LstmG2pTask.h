#ifndef LANGPLUGINS_LSTMG2PTASK_H
#define LANGPLUGINS_LSTMG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>


namespace LangPlugins
{
    class DriverSession;
}

namespace LangPlugins::LstmG2p
{
    namespace Onnx = Api::Onnx::L1;

    class LstmG2pTask : public LangCore::Task {
    public:
        explicit LstmG2pTask(const LangCore::ModuleSpec *spec);
        ~LstmG2pTask() override;

        int apiLevel() const override;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    class LstmG2pInferenceHelper {
    public:
        // Preprocess word into tensor
        static LangCore::Expected<LangCore::NO<ITensor>> preprocessWord(const std::string &word,
                                                                        std::map<std::string, int> charVocab,
                                                                        int bosIdx, int eosIdx, int unkIdx);

        // Get tensor from session result by name
        static LangCore::Expected<LangCore::NO<ITensor>>
        getTensorFromResult(const LangCore::NO<Onnx::SessionResult> &result, const std::string &name);

        // Run decoder with autoregressive generation
        static LangCore::Expected<std::vector<int64_t>>
        runDecoder(const LangCore::NO<LangCore::SessionTask> &decodeSession,
                   const LangCore::NO<ITensor> &encoderOutputs, const LangCore::NO<ITensor> &hidden,
                   const LangCore::NO<ITensor> &cell, int maxLen, int bosIdx, int eosIdx);

        // Decode phoneme indices to phoneme strings
        static LangCore::Expected<std::vector<std::string>>
        decodePhonemes(const std::vector<int64_t> &phonemeIds, const std::map<std::string, int> &phonemeVocab,
                       int bosIdx, int eosIdx, int padIdx, int unkIdx);
    };
} // namespace LangPlugins::LstmG2p

#endif // LANGPLUGINS_LSTMG2PTASK_H
