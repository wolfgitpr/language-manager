#include "TemplateTaggerTask.h"
#include "internal/V1/TaskImpl.h"
#include <LangCore/Support/Logging.h>

namespace LangPlugins::TemplateTagger
{
    TemplateTaggerTask::TemplateTaggerTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::TemplateTaggerTaskImpl>(spec));
    }

    TemplateTaggerTask::~TemplateTaggerTask() = default;

    int TemplateTaggerTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> TemplateTaggerTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateTaggerTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string TemplateTaggerTask::getConfig() const {
        return _manager.getConfig();
    }
} // namespace LangPlugins::TemplateTagger
