#ifndef LANGPLUGINS_LSTMG2P_INTERNAL_V2_TASKIMPL_H
#define LANGPLUGINS_LSTMG2P_INTERNAL_V2_TASKIMPL_H

#include "../TaskImplBase.h"
#include <memory>
#include <shared_mutex>
#include <map>
#include <string>

#include <LangCore/Task/SessionTask.h>
#include <LangCore/Task/TaskFactory.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Core/PackageManager.h>

namespace LangPlugins::LstmG2p::Internal::V2
{
    /// LstmG2p 的 Level 2 实现
    /// 使用 LSTM 模型进行文本到音素的转换（支持批量推理）
    class LstmG2pTaskImpl final : public Internal::LstmG2pTaskImplBase {
    public:
        using Internal::LstmG2pTaskImplBase::LstmG2pTaskImplBase;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;
    };

} // namespace LangPlugins::LstmG2p::Internal::V2

#endif // LANGPLUGINS_LSTMG2P_INTERNAL_V2_TASKIMPL_H