#include <LangCore/Task/TaskPlugin.h>
#include "RegexSplitterTask.h"

using namespace LangCore;
using namespace LangPlugins::RegexSplitter;

LANGCORE_DEFINE_TASK_PLUGIN(
    RegexSplitterPlugin,
    RegexSplitterTask,
    "splitter.regex.RegexSplitterInference",
    1
)
