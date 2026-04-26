#include "LstmG2pTask.h"
#include "internal/V1/TaskImpl.h"
#include "internal/V2/TaskImpl.h"
#include <LangCore/Support/Logging.h>

namespace LangPlugins::LstmG2p
{
    LstmG2pTask::LstmG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        switch (spec->apiLevel()) {
            case 2:
                _manager.setImpl(std::make_unique<Internal::V2::LstmG2pTaskImpl>(spec));
                break;
            case 1:
            default:
                _manager.setImpl(std::make_unique<Internal::V1::LstmG2pTaskImpl>(spec));
                break;
        }
    }

    TASK_IMPLEMENT_METHODS(LstmG2pTask)
} // namespace LangPlugins::LstmG2p
