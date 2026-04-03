#include <LangCore/Task/TaskPlugin.h>
#include "TemplateCleanerTask.h"

using namespace LangCore;
using namespace LangPlugins::TemplateCleaner;

LANGCORE_DEFINE_TASK_PLUGIN(
    TemplateCleanerPlugin,
    TemplateCleanerTask,
    "cleaner.template.TemplateCleanerTask",
    1
)