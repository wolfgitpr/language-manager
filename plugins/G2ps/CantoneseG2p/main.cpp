#include <LangCore/Task/TaskPlugin.h>
#include "CantoneseG2pTask.h"

using namespace LangCore;
using namespace LangPlugins::CantoneseG2p;

LANGCORE_DEFINE_TASK_PLUGIN(
    CantoneseG2pEnginePlugin,
    CantoneseG2pTask,
    "g2p.template.CantoneseG2pInference",
    1
)
