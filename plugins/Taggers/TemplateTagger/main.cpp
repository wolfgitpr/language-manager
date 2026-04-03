#include <LangCore/Task/TaskPlugin.h>
#include "TemplateTaggerTask.h"

using namespace LangCore;
using namespace LangPlugins::TemplateTagger;

LANGCORE_DEFINE_TASK_PLUGIN(
    RegexTaggerInterpreterPlugin,
    TemplateTaggerTask,
    "tagger.template.TemplateTaggerInference",
    1
)
