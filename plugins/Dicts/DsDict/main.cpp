#include <LangCore/Task/TaskPlugin.h>
#include "DsDictTask.h"

using namespace LangCore;
using namespace LangPlugins::DsDict;

LANGCORE_DEFINE_TASK_PLUGIN(
    DsDictPlugin,
    DsDictTask,
    "dict.dsdict",
    1
)