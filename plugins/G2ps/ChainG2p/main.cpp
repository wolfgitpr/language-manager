#include <LangCore/Task/TaskPlugin.h>
#include "ChainG2pTask.h"

using namespace LangCore;
using namespace LangPlugins::ChainG2p;

LANGCORE_DEFINE_TASK_PLUGIN(
    ChainG2pEnginePlugin,
    ChainG2pTask,
    "g2p.chain.ChainG2pInference",
    1
)