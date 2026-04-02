#include "MandarinG2pTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::MandarinG2p
{
    MandarinG2pTask::MandarinG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::MandarinG2pTaskImpl>(spec));
    }

    MandarinG2pTask::~MandarinG2pTask() = default;

    int MandarinG2pTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> MandarinG2pTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    MandarinG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string MandarinG2pTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> MandarinG2pTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::MandarinG2p
