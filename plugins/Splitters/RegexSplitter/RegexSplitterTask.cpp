#include "RegexSplitterTask.h"
#include "internal/V1/TaskImpl.h"

namespace LangPlugins::RegexSplitter
{
    RegexSplitterTask::RegexSplitterTask(const LangCore::ModuleSpec *spec)
        : LangCore::Task(spec), _manager(spec) {
        int level = spec->apiLevel();
        _manager.setCurrentLevel(level);

        // 选择实现（只执行一次，构造函数中）
        // Level 1 作为默认（向下兼容）
        _manager.setImpl(std::make_unique<Internal::V1::RegexSplitterTaskImpl>(spec));
    }

    RegexSplitterTask::~RegexSplitterTask() = default;

    int RegexSplitterTask::apiLevel() const {
        return _manager.currentLevel();
    }

    LangCore::Expected<void> RegexSplitterTask::initialize() {
        return _manager.initialize();
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        return _manager.start(input);
    }

    std::string RegexSplitterTask::getConfig() const {
        return _manager.getConfig();
    }

    LangCore::Expected<void> RegexSplitterTask::setConfig(const std::string &config) {
        return _manager.setConfig(config);
    }
} // namespace LangPlugins::RegexSplitter
