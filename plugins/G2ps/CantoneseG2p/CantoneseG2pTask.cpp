#include "CantoneseG2pTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::CantoneseG2p
{
    CantoneseG2pTask::CantoneseG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::CantoneseG2pTaskImpl>(spec));
    }

    CantoneseG2pTask::~CantoneseG2pTask() = default;

    int CantoneseG2pTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> CantoneseG2pTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    CantoneseG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string CantoneseG2pTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> CantoneseG2pTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::CantoneseG2p
