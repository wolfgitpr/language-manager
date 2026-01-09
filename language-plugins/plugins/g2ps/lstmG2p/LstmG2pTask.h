#ifndef LANG_PLUGINS_LSTMG2PTASK_H
#define LANG_PLUGINS_LSTMG2PTASK_H

#include <../../../../language-manager/include/LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/LstmG2p/1/LstmG2pL1.h>

#include <stdcorelib/str.h>


namespace LangMgr
{
    class DriverSession;
}
namespace LangPlugins
{
    namespace Lstm = Api::LstmG2p::L1;
    namespace Onnx = Api::Onnx::L1;

    class LstmG2pTask : public LangMgr::Task {
    public:
        explicit LstmG2pTask(const LangMgr::ModuleDefinition *definition);
        ~LstmG2pTask() override;

        LangMgr::Expected<void> initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) override;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
        start(const LangMgr::NO<LangMgr::TaskStartInput> &input) override;
        LangMgr::Expected<void> startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                           const StartAsyncCallback &callback) override;
        bool stop() override;

        LangMgr::NO<LangMgr::TaskResult> result() const override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    class LstmG2pInferenceHelper {
    public:
        // Preprocess word into tensor
        static LangMgr::Expected<LangMgr::NO<ITensor>>
        preprocessWord(const std::string &word, const LangMgr::NO<Lstm::LstmG2pConfiguration> &config);

        // Get tensor from session result by name
        static LangMgr::Expected<LangMgr::NO<ITensor>>
        getTensorFromResult(const LangMgr::NO<Onnx::SessionResult> &result, const std::string &name);

        // Run decoder with autoregressive generation
        static LangMgr::Expected<std::vector<int64_t>>
        runDecoder(const LangMgr::NO<LangMgr::SessionTask> &decodeSession, const LangMgr::NO<ITensor> &encoderOutputs,
                   const LangMgr::NO<ITensor> &hidden, const LangMgr::NO<ITensor> &cell,
                   const LangMgr::NO<Lstm::LstmG2pConfiguration> &config);

        // Decode phoneme indices to phoneme strings
        static LangMgr::Expected<std::vector<std::string>>
        decodePhonemes(const std::vector<int64_t> &phonemeIds, const LangMgr::NO<Lstm::LstmG2pConfiguration> &config);
    };
} // namespace LangPlugins

#endif // LANG_PLUGINS_LSTMG2PTASK_H
