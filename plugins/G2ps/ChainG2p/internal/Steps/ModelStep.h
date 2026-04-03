#ifndef LANGPLUGINS_CHAING2P_STEPS_MODELSTEP_H
#define LANGPLUGINS_CHAING2P_STEPS_MODELSTEP_H

#include "../Core/G2pStep.h"
#include <LangCore/Task/Task.h>
#include <LangCore/Task/G2pTask.h>
#include <memory>

namespace LangPlugins::ChainG2p
{
    /// ModelStep - 模型推理步骤
    ///
    /// 使用 AI 模型生成发音
    class ModelStep : public G2pStep {
    public:
        ModelStep() = default;
        ~ModelStep() override = default;

        LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                            const LangCore::JsonObject &config) override;

        void handle(G2pContext &context) override;

        std::string name() const override { return "model"; }

        void cleanup() override;

    private:
        bool m_enabled = true;
        std::string m_onnxG2pId;
        int m_batchSize = 50;
        LangCore::NO<LangCore::Task> m_onnxTask;

        void processBatch(G2pContext &context,
                         const std::vector<size_t> &indices,
                         const std::vector<std::string> &words);
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_STEPS_MODELSTEP_H