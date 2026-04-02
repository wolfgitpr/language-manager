#include "TemplateG2pTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::TemplateG2p
{
    TemplateG2pTask::TemplateG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::TemplateG2pTaskImpl>(spec));
    }

    TemplateG2pTask::~TemplateG2pTask() = default;

    int TemplateG2pTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> TemplateG2pTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string TemplateG2pTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> TemplateG2pTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::TemplateG2p
