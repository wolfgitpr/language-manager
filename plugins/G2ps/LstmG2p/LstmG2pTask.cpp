#include "LstmG2pTask.h"
#include "internal/V1/TaskImpl.h"
#include "internal/V2/TaskImpl.h"

namespace LangPlugins::LstmG2p
{
    LstmG2pTask::LstmG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        if (level == 2) {
            _manager.setImpl(std::make_unique<Internal::V2::LstmG2pTaskImpl>(spec));
        } else {
            // Level 1 作为默认（向下兼容）
            _manager.setImpl(std::make_unique<Internal::V1::LstmG2pTaskImpl>(spec));
        }
    }

    LstmG2pTask::~LstmG2pTask() = default;

    int LstmG2pTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> LstmG2pTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    LstmG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string LstmG2pTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> LstmG2pTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::LstmG2p
