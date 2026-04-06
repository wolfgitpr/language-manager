#include "ChainG2pTask.h"
#include "internal/Core/G2pPipeline.h"
#include "internal/Core/G2pContext.h"
#include "internal/V1/TaskImpl.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>

namespace LangPlugins::ChainG2p
{
    ChainG2pTask::ChainG2pTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);
        // 目前只实现 Level 1
        _manager.setImpl(std::make_unique<Internal::V1::ChainG2pTaskImpl>(spec));
    }

    ChainG2pTask::~ChainG2pTask() = default;

    int ChainG2pTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> ChainG2pTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    ChainG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string ChainG2pTask::getConfig() const {
        return _manager.getConfig();
    }

} // namespace LangPlugins::ChainG2p