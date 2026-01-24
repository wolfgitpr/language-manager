#ifndef LANGPLUGINS_LSTMG2PTASK_H
#define LANGPLUGINS_LSTMG2PTASK_H

#include <LangCore/Task/Task.h>
#include <LangPlugins/Support/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/LstmG2p/1/LstmG2pL1.h>


namespace LangPlugins
{
    class DriverSession;
}
namespace LangPlugins
{
    namespace Lstm = Api::LstmG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class LstmG2pTask : public LangCore::Task {
    public:
        explicit LstmG2pTask(const LangCore::ModuleSpec *spec);
        ~LstmG2pTask() override;

        LangCore::Expected<void> initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskStartInput> &input) override;
        LangCore::Expected<void> startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                            const StartAsyncCallback &callback) override;
        bool stop() override;

        LangCore::NO<LangCore::TaskResult> result() const override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    class LstmG2pInferenceHelper {
    public:
        // Preprocess word into tensor
        static LangCore::Expected<LangCore::NO<ITensor>>
        preprocessWord(const std::string &word, const LangCore::NO<Lstm::LstmG2pConfiguration> &config);

        // Get tensor from session result by name
        static LangCore::Expected<LangCore::NO<ITensor>>
        getTensorFromResult(const LangCore::NO<Onnx::SessionResult> &result, const std::string &name);

        // Run decoder with autoregressive generation
        static LangCore::Expected<std::vector<int64_t>>
        runDecoder(const LangCore::NO<LangCore::SessionTask> &decodeSession,
                   const LangCore::NO<ITensor> &encoderOutputs, const LangCore::NO<ITensor> &hidden,
                   const LangCore::NO<ITensor> &cell, const LangCore::NO<Lstm::LstmG2pConfiguration> &config);

        // Decode phoneme indices to phoneme strings
        static LangCore::Expected<std::vector<std::string>>
        decodePhonemes(const std::vector<int64_t> &phonemeIds, const LangCore::NO<Lstm::LstmG2pConfiguration> &config);
    };
} // namespace LangPlugins

#endif // LANGPLUGINS_LSTMG2PTASK_H
