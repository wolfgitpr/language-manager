#include <LangCore/Task/TaskPlugin.h>
#include "MandarinG2pTask.h"

using namespace LangCore;
using namespace LangPlugins::MandarinG2p;

LANGCORE_DEFINE_TASK_PLUGIN(
    MandarinG2pEnginePlugin,
    MandarinG2pTask,
    "g2p.template.MandarinG2pInference",
    1
)
