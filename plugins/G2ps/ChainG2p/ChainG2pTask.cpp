#include "ChainG2pTask.h"
#include "internal/Core/G2pPipeline.h"
#include "internal/Core/G2pContext.h"
#include "internal/V1/TaskImpl.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>

namespace LangPlugins::ChainG2p
{
    ChainG2pTask::ChainG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        auto impl = std::make_unique<Internal::V1::ChainG2pTaskImpl>(spec);
        impl->setTask(this);
        _manager.setImpl(std::move(impl));
    }

    TASK_IMPLEMENT_METHODS(ChainG2pTask)

} // namespace LangPlugins::ChainG2p