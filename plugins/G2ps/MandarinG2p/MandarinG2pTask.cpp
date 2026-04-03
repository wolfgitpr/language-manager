#include "MandarinG2pTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::MandarinG2p
{
    TASK_IMPLEMENT(MandarinG2pTask, LangCore::VersionedTaskManager<MandarinG2pTask>,
                  Internal::V1, MandarinG2pTaskImpl)

} // namespace LangPlugins::MandarinG2p
