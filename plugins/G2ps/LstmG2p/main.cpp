#include <LangCore/Task/TaskPlugin.h>
#include "LstmG2pTask.h"

using namespace LangCore;
using namespace LangPlugins::LstmG2p;

LANGCORE_DEFINE_TASK_PLUGIN(
    LstmG2pEnginePlugin,
    LstmG2pTask,
    "g2p.model.LstmG2pInference",
    1
)
