#include "DsDictTask.h"
#include "internal/V1/TaskImpl.h"
#include <LangCore/Support/Logging.h>

namespace LangPlugins::DsDict
{
    TASK_IMPLEMENT(DsDictTask, VersionedTaskManager<DsDictTask>, Internal::V1, DsDictTaskImpl)

} // namespace LangPlugins::DsDict