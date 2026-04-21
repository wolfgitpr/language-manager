#include "LangCore/Module/ModuleCategories.h"
#include "Module_p.h"
#include "PackageManager_p.h"

// 定义所有模块类别，触发静态注册
LANGCORE_DEFINE_MODULE_CATEGORY(Driver, "driver")
LANGCORE_DEFINE_MODULE_CATEGORY(G2p, "g2p")
LANGCORE_DEFINE_MODULE_CATEGORY(Splitter, "splitter")
LANGCORE_DEFINE_MODULE_CATEGORY(Tagger, "tagger")
LANGCORE_DEFINE_MODULE_CATEGORY(Dict, "dict")