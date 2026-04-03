#ifndef LANGCORE_CLEANERTASK_H
#define LANGCORE_CLEANERTASK_H

#include <vector>
#include <string>

#include <LangCore/Task/Task.h>

namespace LangCore
{
    /// CleanerInputV1 - 清理器输入 V1
    class CleanerInputV1 : public TaskInput {
    public:
        CleanerInputV1() {}

        std::vector<std::string> cleanerInput;
    };

    /// CleanerResultV1 - 清理器结果 V1
    class CleanerResultV1 : public TaskResult {
    public:
        CleanerResultV1() {}

        std::vector<std::string> cleanerResult;
    };
} // namespace LangCore
#endif // LANGCORE_CLEANERTASK_H