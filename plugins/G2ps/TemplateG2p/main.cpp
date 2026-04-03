#include <LangCore/Task/TaskPlugin.h>
#include "TemplateG2pTask.h"

using namespace LangCore;
using namespace LangPlugins::TemplateG2p;

LANGCORE_DEFINE_TASK_PLUGIN(
    TemplateG2pEnginePlugin,
    TemplateG2pTask,
    "g2p.template.TemplateG2pInference",
    1
)
